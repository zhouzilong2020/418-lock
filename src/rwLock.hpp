#ifndef __RW_LOCK_HPP__
#define __RW_LOCK_HPP__

#include <mutex>

#include "lock.hpp"
class RWLock : public Lock {
   public:
    RWLock() { readerCnt = 0; }
    virtual void lock(const TestContext &ctx) override;
    virtual void unlock(const TestContext &ctx) override;
    virtual std::string getName() override { return name; };
    virtual std::string getHash() override { return hash; };

    virtual ~RWLock() {}

   private:
    std::string name = std::string("RW Lock");
    std::string hash = std::string("RW");
    void lockR();
    void unlockR();
    void lockW();
    void unlockW();
    std::mutex g, r;
    int readerCnt;
};
#endif