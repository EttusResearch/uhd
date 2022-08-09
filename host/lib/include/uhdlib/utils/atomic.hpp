//
// Copyright 2012-2013,2016-2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <atomic>
#include <chrono>

namespace uhd {

/*! DEPRECATED -- May be removed in coming versions of UHD.
 *
 * Claimer class to provide synchronization for multi-thread access.
 *
 * Implements a spinlock mutex with a timeout.
 */
class simple_claimer
{
public:
    simple_claimer(void)
    {
        _locked.clear();
    }

    inline void release(void)
    {
        _locked.clear(std::memory_order_release);
    }

    //!
    // \param timeout Timeout in seconds
    inline bool claim_with_wait(const double timeout)
    {
        const auto exit_time = std::chrono::high_resolution_clock::now()
                               + std::chrono::microseconds(int64_t(timeout * 1e6));
        // Try acquiring lock until successful or timeout
        while (_locked.test_and_set(std::memory_order_acquire)) {
            if (std::chrono::high_resolution_clock::now() > exit_time) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }

private:
    std::atomic_flag _locked;
};

} // namespace uhd
