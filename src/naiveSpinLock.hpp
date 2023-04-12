#ifndef __SPIN_NAIVE_LOCK_HPP__
#define __SPIN_NAIVE_LOCK_HPP__

#include <mutex>

#include "lock.hpp"
class NaiveSpinLock : public Lock {
   public:
    virtual void lock(bool isRead) { mu.lock(); };
    virtual void unlock(bool isRead) { mu.unlock(); };
    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Naive Spin Lock (mutex)");
    std::mutex mu;
};
#endif