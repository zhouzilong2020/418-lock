#ifndef __TICKET_LOCK_HPP__
#define __TICKET_LOCK_HPP__

#include <mutex>
#include <thread>

#include "lock.hpp"

class TicketLock : public Lock {
   public:
    virtual void lock(const TestContext &ctx) {
        uint myTicket = nextTicket.fetch_add(1);
        while (nowServing != myTicket)
            ;
    };

    virtual void unlock(const TestContext &ctx) { nowServing++; };

    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Ticket Lock");
    volatile std::atomic_size_t nowServing = {0};
    char padding[128];  // enough to cover a cache line
    volatile std::atomic_size_t nextTicket = {0};
};
#endif