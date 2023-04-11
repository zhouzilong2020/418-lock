#include "rwLock.hpp"

#include <atomic>

void RWLock::lockR() {
    std::lock_guard<std::mutex> lk(r);
    readerCnt++;
    if (readerCnt == 1) g.lock();
};

void RWLock::unlockR() {
    std::lock_guard<std::mutex> lk(r);
    readerCnt--;
    if (readerCnt == 0) g.unlock();
}

void RWLock::lockW() {
    g.lock();
}

void RWLock::unlockW() {
    g.unlock();
}