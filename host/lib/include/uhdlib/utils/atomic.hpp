//
// Copyright 2012-2013,2016-2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/thread/thread.hpp>
#include <atomic>
#include <chrono>

namespace uhd {

/*! DEPRECATED -- Will be removed in coming versions of UHD.
 *
 * Spin-wait on a condition with a timeout.
 * \param cond an atomic variable to compare
 * \param value compare to atomic for true/false
 * \param timeout the timeout in seconds
 * \return true for cond == value, false for timeout
 */
template <typename T>
UHD_INLINE bool spin_wait_with_timeout(
    std::atomic<T>& cond, const T value, const double timeout)
{
    if (cond == value)
        return true;
    const auto exit_time = std::chrono::high_resolution_clock::now()
                           + std::chrono::microseconds(int64_t(timeout * 1e6));
    while (cond != value) {
        if (std::chrono::high_resolution_clock::now() > exit_time) {
            return false;
        }
        boost::this_thread::interruption_point();
        boost::this_thread::yield();
    }
    return true;
}

/*! DEPRECATED -- Will be removed in coming versions of UHD.
 *
 * Claimer class to provide synchronization for multi-thread access.
 * Claiming enables buffer classes to be used with a buffer queue.
 */
class simple_claimer
{
public:
    simple_claimer(void)
    {
        this->release();
    }

    UHD_INLINE void release(void)
    {
        _locked = false;
    }

    UHD_INLINE bool claim_with_wait(const double timeout)
    {
        if (spin_wait_with_timeout(_locked, false, timeout)) {
            _locked = true;
            return true;
        }
        return false;
    }

private:
    std::atomic<bool> _locked;
};

} // namespace uhd
