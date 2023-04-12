#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

#include "lock.hpp"
#include "naiveSpinLock.hpp"
#include "rwLock.hpp"
#include "spinLock.hpp"
#include "ticketLock.hpp"

using namespace std::chrono;

struct ThreadArgs {
    std::atomic_uint* cnt;
    uint threadCnt;

    const uint readFraction;
    const uint writeFraction;
    const uint iteration;
    std::vector<Lock*> locks;
    ThreadArgs(uint threadCnt, uint readFraction, uint iteration)
        : threadCnt(threadCnt),
          readFraction(readFraction),
          writeFraction(100 - readFraction),
          iteration(iteration) {
        assert(readFraction <= 100 && readFraction >= 0);
        cnt = new std::atomic_uint(0);
    }
};

struct ThreadResult {
    std::vector<double> meanRLatency;
    std::vector<double> meanWLatency;
};

void* worker(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    ThreadResult* res = new ThreadResult();

    for (auto& lock : threadArgs->locks) {
        std::vector<int> rLatency, wLatency;
        int totalWrite = 0, totalRead = 0;
        rLatency.reserve(threadArgs->iteration);
        wLatency.reserve(threadArgs->iteration);

        for (uint i = 0; i < threadArgs->iteration; i++) {
            bool isRead = arc4random() % 100 < threadArgs->readFraction;

            auto start = high_resolution_clock::now();
            if (isRead) {
                totalRead++;
                lock->lock(true);
                usleep(1000);  // 1 ms
                lock->unlock(true);
            } else {
                totalWrite++;
                lock->lock(false);
                usleep(1000);  // 1 ms
                lock->unlock(false);
            }

            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(stop - start);
            if (isRead)
                rLatency.push_back(duration.count() - 1000);
            else
                wLatency.push_back(duration.count() - 1000);
        }

        double wTotal = std::accumulate(wLatency.begin(), wLatency.end(), 0);
        double rTotal = std::accumulate(rLatency.begin(), rLatency.end(), 0);
        res->meanRLatency.push_back(rTotal / rLatency.size());
        res->meanWLatency.push_back(wTotal / wLatency.size());

        (*(threadArgs->cnt))++;
        while (*threadArgs->cnt % threadArgs->threadCnt != 0)
            ;
    }

    pthread_exit(res);
}

int main() {
    const uint threadNum = 16;

    ThreadArgs args(threadNum, 50, 100);
    args.locks.push_back(new SpinLock());
    args.locks.push_back(new NaiveSpinLock());
    args.locks.push_back(new RWLock());
    args.locks.push_back(new TicketLock());

    std::vector<pthread_t> threads;
    for (uint i = 0; i < threadNum; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, worker, &args);
        threads.push_back(pid);
    }

    std::vector<ThreadResult> resList;
    for (uint i = 0; i < threadNum; i++) {
        void* resPtr;
        pthread_join(threads[i], &resPtr);
        resList.push_back(*(ThreadResult*)resPtr);
    }

    for (uint i = 0; i < args.locks.size(); i++) {
        std::cout << args.locks[i]->getName() << std::endl;
        double rVar = 0, wVar = 0, rMean = 0, wMean = 0;

        for (auto& res : resList) {
            rMean += res.meanRLatency[i];
            wMean += res.meanWLatency[i];
        }
        rMean /= resList.size();
        wMean /= resList.size();

        for (auto& res : resList) {
            rVar +=
                (res.meanRLatency[i] - rMean) * (res.meanRLatency[i] - rMean);
            wVar +=
                (res.meanWLatency[i] - wMean) * (res.meanWLatency[i] - wMean);
        }
        rVar = sqrt(rVar / resList.size());
        wVar = sqrt(wVar / resList.size());

        std::cout << "Read Latency  | mean: " << rMean << " us var: " << rVar
                  << std::endl;
        std::cout << "Write Latency | mean: " << wMean << " us var: " << wVar
                  << std::endl;
    }
}