#ifndef __SPIN_LOCK_HPP__
#define __SPIN_LOCK_HPP__

#include <mutex>

#include "lock.hpp"
class SpinLock : public Lock {
   public:
    virtual void lock(bool isRead) { mu.lock(); };
    virtual void unlock(bool isRead) { mu.unlock(); };
    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Spin Lock (test and set)");
    // std::atomic_bool isLock;
    std::mutex mu;
};
#endif