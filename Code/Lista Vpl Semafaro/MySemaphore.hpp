#ifndef MYSEMAPHORE
#define MYSEMAPHORE

#include "AbstractSemaphore.hpp"
#include <iostream>
#include <mutex>
#include <condition_variable>

class MySemaphore : public AbstractSemaphore
{
private:
    std::mutex mtx;
    std::condition_variable cv;

public:
    MySemaphore(int cap = 1) : AbstractSemaphore(cap) {}

    void release() override {
        std::lock_guard<std::mutex> lock(mtx);
        m_nfree++;
        cv.notify_one(); // Notify one waiting thread
    }

    void acquire() override {
        std::unique_lock<std::mutex> lock(mtx);
        while (m_nfree == 0) {
            cv.wait(lock); // Wait until semaphore becomes available
        }
        m_nfree--;
    }
};

#endif
