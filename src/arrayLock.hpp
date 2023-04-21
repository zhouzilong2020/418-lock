#ifndef __ARRAY_LOCK_HPP__
#define __ARRAY_LOCK_HPP__

#include <pthread.h>

#include <atomic>
#include <string>
#include <thread>

#include "lock.hpp"
class ArrayLock : public Lock {
   public:
    virtual void lock(const TestContext& ctx) {
        auto myEle = head.fetch_add(1, std::memory_order_acquire) % threadCnt;
        setMyEle(const_cast<TestContext*>(&ctx), myEle);
        while (status[myEle].value.load() == 1) {
            std::this_thread::yield();
        }
    }

    virtual void unlock(const TestContext& ctx) {
        status[ctx.myEle].value.store(1, std::memory_order_release);
        status[(ctx.myEle + 1) % threadCnt].value.store(
            0, std::memory_order_release);
    };

    virtual std::string getName() { return name; };

    ArrayLock(const unsigned int threadCnt) {
        this->threadCnt = threadCnt;
        status = new PaddedInt[threadCnt];
        for (unsigned int i = 0; i < threadCnt; i++) {
            status[i].value.store(1);
        }
        status[0].value.store(0);
    }
    ~ArrayLock() { delete status; }

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