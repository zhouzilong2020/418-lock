#ifndef __MY__LOCK_HPP__
#define __MY__LOCK_HPP__
#include <string>
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-private-field"

struct TestContext {
    const bool isRead;  // for RW lock
    int myEle;          // for array lock
    TestContext(const bool isRead) : isRead(isRead), myEle(0){};
};

typedef struct TestContext TestContext;

class Lock {
   public:
    virtual void lock(const TestContext &ctx) = 0;
    virtual void unlock(const TestContext &ctx) = 0;
    virtual std::string getName() = 0;
};
#endif