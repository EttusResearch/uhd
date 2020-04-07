//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace uhd {

/*!
 * A sempahore built using std::condition_variable
 */
class semaphore
{
public:
    void notify()
    {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        _count++;
        _cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        _cv.wait(lock, [this]() { return this->_count != 0; });
        _count--;
    }

    bool try_wait()
    {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        if (_count != 0) {
            _count--;
            return true;
        }
        return false;
    }

    bool wait_for(size_t timeout_ms)
    {
        std::chrono::milliseconds timeout(timeout_ms);
        std::unique_lock<std::mutex> lock(_cv_mutex);
        if (_cv.wait_for(lock, timeout, [this]() { return this->_count != 0; })) {
            _count--;
            return true;
        }
        return false;
    }

    size_t count()
    {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        return _count;
    }

private:
    std::condition_variable _cv;
    std::mutex _cv_mutex;
    size_t _count = 0;
};

} // namespace uhd
