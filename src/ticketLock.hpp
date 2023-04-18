#ifndef __TICKET_LOCK_HPP__
#define __TICKET_LOCK_HPP__

#include <mutex>
#include <thread>

#include "lock.hpp"

class TicketLock : public Lock {
   public:
    virtual void lock(const TestContext &ctx) {
        uint myTicket = nextTicket.fetch_add(1, std::memory_order_relaxed);

        while (myTicket != nowServing.load(std::memory_order_acquire))
            ;
    };

    virtual void unlock(const TestContext &ctx) {
        nowServing.fetch_add(1, std::memory_order_release);
    };

    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Ticket Lock");
    std::atomic_size_t nowServing = {0};
    char padding[128];  // enough to cover a cache line
    std::atomic_size_t nextTicket = {0};
};
#endif