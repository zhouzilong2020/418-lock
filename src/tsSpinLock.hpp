#ifndef __TS_SPIN_LOCK_HPP__
#define __TS_SPIN_LOCK_HPP__

#include <atomic>
#include <thread>

#include "lock.hpp"
class TSSpinLock : public Lock {
   public:
    TSSpinLock() { isLocked.clear(); }
    virtual void lock(const TestContext &ctx) override {
        while (isLocked.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    };
    virtual void unlock(const TestContext &ctx) override {
        isLocked.clear(std::memory_order_release);
    };
    virtual std::string getName() override { return name; };
    virtual std::string getHash() override { return hash; };
    virtual ~TSSpinLock(){};

   private:
    std::string name = std::string("Spin Lock (test and set)");
    std::string hash = std::string("TS");
    volatile std::atomic_flag isLocked;
};

#endif