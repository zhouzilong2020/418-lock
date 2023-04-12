#ifndef __TICKET_LOCK_HPP__
#define __TICKET_LOCK_HPP__

#include <mutex>
#include <thread>

#include "lock.hpp"

class TicketLock : public Lock {
   public:
    virtual void lock(bool isRead) {
        uint myTicket = nextTicket.fetch_add(1);
        while (nowServing != myTicket)
            ;
    };

    virtual void unlock(bool isRead) { nowServing++; };

    virtual std::string getName() { return name; };

   private:
    std::string name = std::string("Ticket Lock");
    std::atomic_size_t nowServing = {0};
    char padding[128];  // enough to cover a cache line
    std::atomic_size_t nextTicket = {0};
};
#endif