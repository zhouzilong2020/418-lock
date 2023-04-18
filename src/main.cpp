#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

#include "arrayLock.hpp"
#include "lock.hpp"
#include "naiveSpinLock.hpp"
#include "rwLock.hpp"
#include "ticketLock.hpp"
#include "tsSpinLock.hpp"
#include "ttsSpinLock.hpp"

using namespace std::chrono;

struct ThreadArgs {
    std::atomic_uint* cnt;
    uint threadCnt;
    const double wFrac;
    const uint iteration;
    std::vector<Lock*> locks;
    std::vector<bool> testCase;
    uint totalWrite;

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
    ThreadArgs(uint threadCnt, double wFrac, uint iteration)
        : threadCnt(threadCnt), wFrac(wFrac), iteration(iteration) {
        readyCnt = 0;
        finishCnt = 0;
        totalWrite = 0;
        cnt = new std::atomic_uint(0);
        pthread_cond_init(&readyCond, NULL);
        pthread_mutex_init(&readyMutex, NULL);
        testCase.resize(iteration, false);
        for (uint i = 0; i < iteration; i++) {
            if (rand() % 100 < wFrac * 100) {
                testCase[i] = true;
                totalWrite++;
            }
        }
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
            bool isWrite = threadArgs->testCase[j];
            auto start = high_resolution_clock::now();
            if (!isWrite) {
                totalRead++;
                lock->lock(rCtx);
                // some read
                for (int i = 0; i < 100; i++)
                    ;
                lock->unlock(rCtx);
            } else {
                totalWrite++;
                lock->lock(wCtx);
                threadArgs->counter[i]++;
                // some write
                for (int i = 0; i < 10000; i++)
                    ;
                lock->unlock(wCtx);
            }

            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(stop - start);
            if (!isWrite)
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
    const uint threadNum = 8;
    ThreadArgs args(threadNum, 0.1, 100000);
    args.addLock(new NaiveSpinLock());
    args.addLock(new TSSpinLock());
    args.addLock(new TTSSpinLock());
    args.addLock(new RWLock());
    args.addLock(new TicketLock());
    args.addLock(new ArrayLock(threadNum));

    std::vector<pthread_t> threads;
    for (uint i = 0; i < threadNum; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, worker, &args);
        threads.push_back(pid);
    }

    std::vector<double> duration(args.locks.size(), 0);
    for (uint i = 0; i < args.locks.size(); i++) {
        // wait until all worker are ready
        while (args.readyCnt != threadNum)
            ;
        printf("begin testing (%d/%lu) %s\n", i + 1, args.locks.size(),
               args.locks[i]->getName().c_str());
        args.readyCnt = 0;

        auto start = high_resolution_clock::now();
        pthread_cond_broadcast(&args.readyCond);
        while (args.finishCnt != threadNum) std::this_thread::yield();
        auto stop = high_resolution_clock::now();
        duration[i] = duration_cast<milliseconds>(stop - start).count();

        args.finishCnt = 0;
        // check if the lock is correct
        uint expected = threadNum * args.totalWrite;
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

        std::cout << args.locks[i]->getName() << std::endl;
        std::cout << std::fixed << std::setprecision(2)
                  << "Total time elapsed: " << duration[i] << "ms" << std::endl;
        std::cout << std::fixed << std::setprecision(2)
                  << "Read Latency  | mean: " << rMean << "us var: " << rVar
                  << std::endl;
        std::cout << std::fixed << std::setprecision(2)
                  << "Write Latency | mean: " << wMean << "us var: " << wVar
                  << std::endl
                  << std::endl;
    }
}