#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "arrayLock.hpp"
#include "lock.hpp"
#include "naiveSpinLock.hpp"
#include "rwLock.hpp"
#include "ticketLock.hpp"
#include "time.hpp"
#include "tsSpinLock.hpp"
#include "ttsSpinLock.hpp"

using namespace std::chrono;

struct ThreadArgs {
    std::atomic_uint* cnt;

    uint threadCnt;
    double wFrac;
    uint iteration;
    std::vector<Lock*> locks;
    std::vector<bool> testCase;
    uint totalWrite;

    uint writeTime;  // simulate write, the higher the more expensive the op is
    uint readTime;   // simulate read, the higher the more expensive the op is

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
    void addLock(const std::vector<Lock*>& _locks) {
        counter.resize(_locks.size());
        locks = _locks;
    }
    ThreadArgs(){};
    ThreadArgs(uint threadCnt, double wFrac, uint iteration,
               uint writeTime = 1000, uint readTime = 100)
        : threadCnt(threadCnt),
          wFrac(wFrac),
          iteration(iteration),
          writeTime(writeTime),
          readTime(readTime) {
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
    ~ThreadArgs() {
        delete cnt;
        for (auto& lock : locks) {
            delete lock;
        }
    }
    friend std::ostream& operator<<(std::ostream& os, const ThreadArgs& ta);
    std::string hash() {
        char buf[64];
        sprintf(buf, "tc_%d-wf_%.1f-wt_%d-rt_%d", threadCnt, wFrac, writeTime,
                readTime);
        return buf;
    }
};

std::ostream& operator<<(std::ostream& os, const ThreadArgs& ta) {
    os << "thread number: " << ta.threadCnt << std::endl
       << "write fraction: " << ta.wFrac << std::endl
       << "read fraction: " << 1.0 - ta.wFrac << std::endl
       << "write time: " << ta.writeTime << std::endl
       << "read time: " << ta.readTime;
    return os;
}

typedef std::unordered_map<
    std::string, std::pair<std::vector<int64_t>, std::vector<int64_t>>>
    ThreadResult;

void* worker(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    ThreadResult* res = new ThreadResult();
    TestContext rCtx(true), wCtx(false);
    uint writeTime = threadArgs->writeTime;
    uint readTime = threadArgs->readTime;

    for (uint i = 0; i < threadArgs->locks.size(); i++) {
        auto& lock = threadArgs->locks[i];
        std::vector<int64_t> rLatency, wLatency;
        rLatency.reserve(threadArgs->iteration);
        wLatency.reserve(threadArgs->iteration);

        // synchronization
        pthread_mutex_lock(&threadArgs->readyMutex);
        threadArgs->readyCnt++;
        pthread_cond_wait(&threadArgs->readyCond, &threadArgs->readyMutex);
        pthread_mutex_unlock(&threadArgs->readyMutex);

        high_resolution_clock::time_point start, stop;
        uint localCnt = 0, foo = 0;
        for (uint j = 0; j < threadArgs->iteration; j++) {
            bool isWrite = threadArgs->testCase[j];
            if (isWrite) {
                start = high_resolution_clock::now();
                lock->lock(wCtx);
                stop = high_resolution_clock::now();
                localCnt++;

                // some heavy write
                for (uint i = 0; i < writeTime; i++)
                    ;
                lock->unlock(wCtx);
            } else {
                start = high_resolution_clock::now();
                lock->lock(rCtx);
                stop = high_resolution_clock::now();
                foo++;  // because we increment localCnt in write, this
                        // guarantees that the only difference between write and
                        // read is caused by threadParameter

                // some light read
                for (uint i = 0; i < readTime; i++)
                    ;
                lock->unlock(rCtx);
            }

            auto duration = duration_cast<nanoseconds>(stop - start).count();
            if (isWrite)
                wLatency.push_back(duration);
            else
                rLatency.push_back(duration);
        }

        lock->lock(wCtx);
        threadArgs->counter[i] += localCnt;
        lock->unlock(wCtx);

        (*res)[lock->getHash()].first = rLatency;
        (*res)[lock->getHash()].second = wLatency;

        // synchronization
        pthread_mutex_lock(&threadArgs->readyMutex);
        threadArgs->finishCnt++;
        pthread_mutex_unlock(&threadArgs->readyMutex);
    }

    pthread_exit(res);
}

void run(ThreadArgs& args, const std::string& outputDir) {
    std::vector<pthread_t> threads;
    for (uint i = 0; i < args.threadCnt; i++) {
        pthread_t pid;
        pthread_create(&pid, NULL, worker, (void*)&args);
        threads.push_back(pid);
    }

    std::cout << args << std::endl;
    std::vector<int64_t> duration(args.locks.size(), 0);
    for (uint i = 0; i < args.locks.size(); i++) {
        // wait until all worker are ready
        while (args.readyCnt != args.threadCnt) {
            std::this_thread::yield();
        }

        printf("begin testing (%d/%lu) %s\n", i + 1, args.locks.size(),
               args.locks[i]->getName().c_str());
        args.readyCnt = 0;

        auto start = high_resolution_clock::now();
        pthread_cond_broadcast(&args.readyCond);
        while (args.finishCnt != args.threadCnt) std::this_thread::yield();
        auto stop = high_resolution_clock::now();
        duration[i] = duration_cast<milliseconds>(stop - start).count();

        args.finishCnt = 0;
        // check if the lock is correct
        uint expected = args.threadCnt * args.totalWrite;
        if (args.counter[i] != expected) {
            fprintf(stderr, "%s is incorrect! expected %d got %d\n",
                    args.locks[i]->getName().c_str(), expected,
                    args.counter[i]);
            exit(EXIT_FAILURE);
        }
    }

    std::vector<ThreadResult> resList;
    for (uint i = 0; i < args.threadCnt; i++) {
        void* resPtr;
        pthread_join(threads[i], &resPtr);
        resList.push_back(*(ThreadResult*)resPtr);
    }

    // dumping result to local file
    std::ofstream file;
    char filePath[64];
    for (auto& lock : args.locks) {
        sprintf(filePath, "%s/%s-r-%s", outputDir.c_str(),
                lock->getHash().c_str(), args.hash().c_str());
        file.open(filePath, std::ios::trunc);
        for (auto& res : resList) {
            auto rLatency = res[lock->getHash()].first;
            for (auto& num : rLatency) file << num << " ";
        }
        file.close();

        sprintf(filePath, "%s/%s-w-%s", outputDir.c_str(),
                lock->getHash().c_str(), args.hash().c_str());
        file.open(filePath, std::ios::trunc);
        for (auto& res : resList) {
            auto wLatency = res[lock->getHash()].second;
            for (auto& num : wLatency) file << num << " ";
        }
        file.close();
    }
}

int main(int argc, char* argv[]) {
    uint threadNum = 0;
    double wFrac = 0.;
    uint writeTime = 0;
    uint readTime = 0;
    char* outputDir = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "t:w:W:r:o:n")) != -1) {
        switch (opt) {
        case 't':
            threadNum = atoi(optarg);
            break;
        case 'w':
            wFrac = atof(optarg);
            break;
        case 'W':
            writeTime = atoi(optarg);
            break;
        case 'r':
            readTime = atoi(optarg);
            break;
        case 'o':
            outputDir = optarg;
            break;
        default:
            std::cerr << "Usage: " << argv[0]
                      << " -t <number of thread>"
                         " -w <write fraction 0-1>"
                         " -W <write time>"
                         " -r <read time>"
                         " -o <output dir>\n";
            return 1;
        }
    }
    if (threadNum == 0 || wFrac == 0.0 || writeTime == 0 || readTime == 0 ||
        outputDir == NULL) {
        std::cerr << "Missing required arguments.\n";
        std::cerr << "Usage: " << argv[0]
                  << " -t <number of thread>"
                     " -w <write fraction 0-1>"
                     " -W <write time>"
                     " -r <read time>"
                     " -o <output dir>\n";
        return 1;
    }

    ThreadArgs args(threadNum, wFrac, 10000 /* itr */,
                    writeTime /* write time*/, readTime /* read time */);
    args.addLock({new NaiveSpinLock(), new TSSpinLock(), new TTSSpinLock(),
                  new RWLock(), new TicketLock(), new ArrayLock(threadNum)});
    run(args, outputDir);
}