#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

#include "arrayLock.hpp"
#include "lock.hpp"
#include "naiveSpinLock.hpp"
#include "rwLock.hpp"
#include "spinLock.hpp"
#include "ticketLock.hpp"

using namespace std::chrono;

struct ThreadArgs {
    std::atomic_uint* cnt;
    uint threadCnt;
    const uint rwRatio;
    const uint iteration;
    std::vector<Lock*> locks;

    // counter is used to test the correctness of the lock
    std::vector<uint> counter;
    // readyCnt and readyCond are used to sync the worker thread.
    // the master thread will broadcast the readyCond when all the worker are
    // ready and reset the readyCnt.
    volatile uint readyCnt;
    volatile uint finishCnt;
    pthread_mutex_t readyMutex;
    pthread_cond_t readyCond;

    void addLock(Lock* lock) {
        locks.push_back(lock);
        counter.resize(locks.size());
    }
    ThreadArgs(uint threadCnt, uint rwRatio, uint iteration)
        : threadCnt(threadCnt), rwRatio(rwRatio), iteration(iteration) {
        readyCnt = 0;
        finishCnt = 0;
        cnt = new std::atomic_uint(0);
        pthread_cond_init(&readyCond, NULL);
        pthread_mutex_init(&readyMutex, NULL);
    }
};

struct ThreadResult {
    std::vector<double> meanRLatency;
    std::vector<double> meanWLatency;
};

void* worker(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    ThreadResult* res = new ThreadResult();
    TestContext rCtx(true), wCtx(false);

    for (uint i = 0; i < threadArgs->locks.size(); i++) {
        auto& lock = threadArgs->locks[i];
        std::vector<int> rLatency, wLatency;
        int totalWrite = 0, totalRead = 0;
        rLatency.reserve(threadArgs->iteration);
        wLatency.reserve(threadArgs->iteration);

        // synchronization
        pthread_mutex_lock(&threadArgs->readyMutex);
        threadArgs->readyCnt++;
        pthread_cond_wait(&threadArgs->readyCond, &threadArgs->readyMutex);
        pthread_mutex_unlock(&threadArgs->readyMutex);

        for (uint j = 0; j < threadArgs->iteration; j++) {
            bool isRead = !(j % threadArgs->rwRatio);

            auto start = high_resolution_clock::now();
            if (isRead) {
                totalRead++;
                lock->lock(rCtx);
                lock->unlock(rCtx);
            } else {
                totalWrite++;
                lock->lock(wCtx);
                threadArgs->counter[i]++;
                lock->unlock(wCtx);
            }

            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(stop - start);
            if (isRead)
                rLatency.push_back(duration.count());
            else
                wLatency.push_back(duration.count());
        }

        double wTotal = std::accumulate(wLatency.begin(), wLatency.end(), 0);
        double rTotal = std::accumulate(rLatency.begin(), rLatency.end(), 0);
        res->meanRLatency.push_back(rTotal / rLatency.size());
        res->meanWLatency.push_back(wTotal / wLatency.size());

        // synchronization
        pthread_mutex_lock(&threadArgs->readyMutex);
        threadArgs->finishCnt++;
        pthread_mutex_unlock(&threadArgs->readyMutex);
    }

    pthread_exit(res);
}

int main() {
    const uint threadNum = 12;
    ThreadArgs args(threadNum, 10, 10000);
    args.addLock(new SpinLock());
    args.addLock(new NaiveSpinLock());
    args.addLock(new RWLock());
    args.addLock(new TicketLock());
    args.addLock(new ArrayLock(threadNum));

    std::vector<pthread_t> threads;
    for (uint i = 0; i < threadNum; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, worker, &args);
        threads.push_back(pid);
    }

    for (uint i = 0; i < args.locks.size(); i++) {
        // wait until all worker are ready
        while (args.readyCnt != threadNum)
            ;
        args.readyCnt = 0;
        pthread_cond_broadcast(&args.readyCond);
        while (args.finishCnt != threadNum)
            ;
        args.finishCnt = 0;
        // check if the lock is correct
        uint expected =
            threadNum * args.iteration * (1 - 1.0 / (double)args.rwRatio);
        if (args.counter[i] != expected) {
            fprintf(stderr, "%s is incorrect! expected %d got %d\n",
                    args.locks[i]->getName().c_str(), expected,
                    args.counter[i]);
            return 1;
        }
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