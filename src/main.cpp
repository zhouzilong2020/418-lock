#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

using namespace std::chrono;

#include "lock.hpp"
#include "rwLock.hpp"
#include "spinLock.hpp"
#include "ticketLock.hpp"

struct ThreadArgs {
    const uint readFraction;
    const uint writeFraction;
    const int iteration;
    std::vector<Lock*> locks;
    ThreadArgs(int readFraction, int iteration)
        : readFraction(readFraction),
          writeFraction(100 - readFraction),
          iteration(iteration) {}
};

void* worker(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;

    for (auto lock : threadArgs->locks) {
        std::vector<int> rLatency, wLatency;
        int totalWrite = 0, totalRead = 0;
        rLatency.reserve(threadArgs->iteration);
        wLatency.reserve(threadArgs->iteration);

        for (int i = 0; i < threadArgs->iteration; i++) {
            bool isRead = false;
            auto start = high_resolution_clock::now();

            if (arc4random() % 100 < threadArgs->readFraction) {
                totalRead++;
                lock->lock(true);
                usleep(1000);  // 1 ms
                lock->unlock(true);
                isRead = true;
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
        printf("[%s] mean read latency: %.2f, write latency: %.2f\n",
               lock->getName().c_str(), wTotal / wLatency.size(),
               rTotal / rLatency.size());
    }
    return NULL;
}

int main() {
    ThreadArgs args(50, 100);
    args.locks.push_back(new SpinLock());
    // args.locks.push_back(new RWLock());
    // args.locks.push_back(new TicketLock());
    // args.locks[std::string("Spin Lock")] = &std::mutex();

    std::vector<pthread_t> threads;
    uint threadNum = 16;
    for (uint i = 0; i < threadNum; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, worker, &args);
        threads.push_back(pid);
    }

    for (uint i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }
}