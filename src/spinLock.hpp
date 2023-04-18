#ifndef __SPIN_LOCK_HPP__
#define __SPIN_LOCK_HPP__

#include "lock.hpp"
class SpinLock : public Lock {
   public:
    SpinLock() { isLocked.store(false); }
    virtual void lock(const TestContext &ctx) {
        while (1) {
            while (isLocked.load(std::memory_order_acquire))
                ;
            if (!isLocked.exchange(true, std::memory_order_acquire)) return;
        }
    };
    virtual void unlock(const TestContext &ctx) {
        isLocked.store(false, std::memory_order_release);
    };
    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Spin Lock (test and test and set)");
    volatile std::atomic_bool isLocked;
};

#endif