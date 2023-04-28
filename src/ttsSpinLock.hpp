#ifndef __TTS_SPIN_LOCK_HPP__
#define __TTS_SPIN_LOCK_HPP__

#include <atomic>
#include <thread>

#include "lock.hpp"
class TTSSpinLock : public Lock {
   public:
    TTSSpinLock() { isLocked.store(ATOMIC_FLAG_INIT); }
    virtual void lock(const TestContext &ctx) override {
        while (1) {
            while (isLocked.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (!isLocked.exchange(true, std::memory_order_acquire)) return;
        }
    };
    virtual void unlock(const TestContext &ctx) override {
        isLocked.store(false, std::memory_order_release);
    };
    virtual std::string getName() override { return name; };
    virtual std::string getHash() override { return hash; };
    virtual ~TTSSpinLock(){};

   private:
    std::string name = std::string("Spin Lock (test and test and set)");
    std::string hash = std::string("TTS");
    volatile std::atomic_bool isLocked;
};

#endif