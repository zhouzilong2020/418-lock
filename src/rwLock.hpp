#ifndef __RW_LOCK_HPP__
#define __RW_LOCK_HPP__

#include <mutex>

class RWLock {
   public:
    RWLock() { readerCnt = 0; }
    void lockR();
    void unlockR();
    void lockW();
    void unlockW();

   private:
    std::mutex g, r;
    int readerCnt;
};
#endif