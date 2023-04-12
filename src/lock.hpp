#ifndef __MY__LOCK_HPP__
#define __MY__LOCK_HPP__

class Lock {
   public:
    virtual void lock(bool isRead) = 0;
    virtual void unlock(bool isRead) = 0;
    virtual std::string getName() = 0;
};
#endif