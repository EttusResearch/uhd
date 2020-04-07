//
// Copyright 2010,2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <boost/thread/thread.hpp>
#include <string>
#include <thread>

namespace uhd {

constexpr float DEFAULT_THREAD_PRIORITY = float(0.5);

/*!
 * Set the scheduling priority on the current thread.
 *
 * To enable realtime scheduling on a new thread, call this function with
 * the default values. Warning: realtime scheduling can cause UHD worker
 * threads to not share resources such as network interfaces fairly,
 * potentially causing it to malfunction.
 *
 * A priority of zero corresponds to normal priority.
 * Positive priority values are higher than normal.
 * Negative priority values are lower than normal.
 *
 * \param priority a value between -1 and 1
 * \param realtime true to use realtime mode
 * \throw exception on set priority failure
 */
UHD_API void set_thread_priority(
    float priority = DEFAULT_THREAD_PRIORITY, bool realtime = true);

/*!
 * Set the scheduling priority on the current thread.
 * Same as set_thread_priority but does not throw on failure.
 * \return true on success, false on failure
 */
UHD_API bool set_thread_priority_safe(
    float priority = DEFAULT_THREAD_PRIORITY, bool realtime = true);

/*!
 * Set the thread name on the given boost thread.
 * \param thread pointer to a boost thread
 * \param name thread name with maximum length of 16 characters
 */
UHD_API void set_thread_name(boost::thread* thread, const std::string& name);

/*!
 * Set the thread name on the given std thread.
 * \param thread pointer to a boost thread
 * \param name thread name with maximum length of 16 characters
 */
UHD_API void set_thread_name(std::thread* thread, const std::string& name);

/*!
 * Set the affinity of the current thread to a (set of) CPU(s).
 * \param cpu_affinity_list list of CPU numbers to affinitize the thread to
 */
UHD_API void set_thread_affinity(const std::vector<size_t>& cpu_affinity_list);

} // namespace uhd
