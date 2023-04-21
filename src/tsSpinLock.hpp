#ifndef __TS_SPIN_LOCK_HPP__
#define __TS_SPIN_LOCK_HPP__

#include <atomic>
#include <thread>

#include "lock.hpp"
class TSSpinLock : public Lock {
   public:
    TSSpinLock() { isLocked.clear(); }
    virtual void lock(const TestContext &ctx) {
        while (isLocked.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    };
    virtual void unlock(const TestContext &ctx) {
        isLocked.clear(std::memory_order_release);
    };
    virtual std::string getName() { return name; };
    virtual ~TSSpinLock(){};

   private:
    std::string name = std::string("Spin Lock (test and set)");
    volatile std::atomic_flag isLocked;
};

#endif