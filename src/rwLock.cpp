#include "rwLock.hpp"

#include <atomic>

void RWLock::lock(const TestContext &ctx) {
    if (ctx.isRead) {
        lockR();
        return;
    }
    lockW();
};

void RWLock::unlock(const TestContext &ctx) {
    if (ctx.isRead) {
        unlockR();
        return;
    }
    unlockW();
};

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