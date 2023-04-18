#ifndef __ARRAY_LOCK_HPP__
#define __ARRAY_LOCK_HPP__

#include <pthread.h>

#include <atomic>
#include <string>

#include "lock.hpp"
class ArrayLock : public Lock {
   public:
    virtual void lock(const TestContext& ctx) {
        // atomic circular increment
        int oldHead = head;
        int newHead = (oldHead + 1) % threadCnt;
        while (!head.compare_exchange_weak(oldHead, newHead))
            ;
        setMyEle(const_cast<TestContext*>(&ctx), oldHead);

        while (status[oldHead].value.load() == 1)
            ;
    }

    virtual void unlock(const TestContext& ctx) {
        status[ctx.myEle].value.store(1);
        status[(ctx.myEle + 1) % threadCnt].value.store(0);
    };

    virtual std::string getName() { return name; };

    ArrayLock(const unsigned int threadCnt) {
        this->threadCnt = threadCnt;
        status = new PaddedInt[threadCnt];
        for (unsigned int i = 0; i < threadCnt; i++) {
            status[i].value.store(0);
        }
    }

   private:
    void setMyEle(TestContext* ctx, int head) { ctx->myEle = head; }

    typedef struct {
        volatile std::atomic_int value;
        char padding[128];
    } PaddedInt;

    unsigned int threadCnt;
    std::string name = std::string("Array Lock");
    PaddedInt* status;
    volatile std::atomic_int head;
};
#endif