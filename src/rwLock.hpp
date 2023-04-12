#ifndef __RW_LOCK_HPP__
#define __RW_LOCK_HPP__

#include <mutex>

#include "lock.hpp"
class RWLock : public Lock {
   public:
    RWLock() { readerCnt = 0; }
    virtual void lock(bool isRead);
    virtual void unlock(bool isRead);
    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("RW Lock");
    void lockR();
    void unlockR();
    void lockW();
    void unlockW();
    std::mutex g, r;
    int readerCnt;
};
#endif