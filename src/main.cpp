#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

using namespace std::chrono;

#include "rwLock.hpp"

struct ThreadArgs {
    int readFraction;
    int writeFraction;
    int iteration;
    RWLock* lock;
    ThreadArgs(int readFraction, int iteration, RWLock* lock)
        : readFraction(readFraction),
          writeFraction(100 - readFraction),
          iteration(iteration),
          lock(lock) {}
};

void* workerThread(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;

    std::vector<int> latency;
    int totalWrite = 0, totalRead = 0;
    latency.reserve(threadArgs->iteration);
    for (int i = 0; i < threadArgs->iteration; i++) {
        auto start = high_resolution_clock::now();

        if (arc4random() % 100 < threadArgs->readFraction) {
            totalRead++;
            threadArgs->lock->lockR();
            usleep(100);  // 100 us
            threadArgs->lock->unlockR();
        } else {
            totalWrite++;
            threadArgs->lock->lockW();
            usleep(100);  // 100 us
            threadArgs->lock->unlockW();
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        latency.push_back(duration.count() - 100);
    }

    double total = std::accumulate(latency.begin(), latency.end(), 0);
    printf("mean latency (R %d, W %d): %.2f\n", totalRead, totalWrite,
           total / latency.size());

    return NULL;
}

int main(int argc, char* argv[]) {
    RWLock rwLock;
    std::mutex spinlock;
    ThreadArgs args(70, 10000, &rwLock);

    std::vector<pthread_t> threads;
    for (int i = 0; i < 8; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, workerThread, &args);
        threads.push_back(pid);
    }

    for (int i = 0; i < 16; i++) {
        pthread_join(threads[i], NULL);
    }
}