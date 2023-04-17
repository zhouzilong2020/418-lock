#ifndef __SPIN_LOCK_HPP__
#define __SPIN_LOCK_HPP__

#include "lock.hpp"
class SpinLock : public Lock {
   public:
    SpinLock() { isLock.store(false); }
    virtual void lock(const TestContext &ctx) {
        while (1) {
            while (isLock != false)
                ;
            if (isLock.compare_exchange_strong(F, true)) return;
        }
    };
    virtual void unlock(const TestContext &ctx) { isLock.store(false); };
    virtual std::string getName() { return name; };

   private:
    bool F = false;
    std::string name = std::string("Spin Lock (test and set)");
    volatile std::atomic_bool isLock;
};
#endif