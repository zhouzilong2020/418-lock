#ifndef __SPIN_NAIVE_LOCK_HPP__
#define __SPIN_NAIVE_LOCK_HPP__

#include <mutex>

#include "lock.hpp"
class NaiveSpinLock : public Lock {
   public:
    virtual void lock(const TestContext &ctx) override { mu.lock(); };
    virtual void unlock(const TestContext &ctx) override { mu.unlock(); };
    virtual std::string getName() override { return name; };
    virtual std::string getHash() override { return hash; };

   private:
    std::string name = std::string("Mutex");
    std::string hash = std::string("M");
    std::mutex mu;
};
#endif