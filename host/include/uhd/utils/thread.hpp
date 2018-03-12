//
// Copyright 2010,2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_THREAD_HPP
#define INCLUDED_UHD_UTILS_THREAD_HPP

#include <uhd/config.hpp>
#include <boost/thread/thread.hpp>
#include <string>

namespace uhd{

    static const float default_thread_priority = float(0.5);

    /*!
     * Set the scheduling priority on the current thread.
     *
     * A new thread or calling process should make this call
     * with the defaults this to enable realtime scheduling.
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
        float priority = default_thread_priority,
        bool realtime = true
    );

    /*!
     * Set the scheduling priority on the current thread.
     * Same as set_thread_priority but does not throw on failure.
     * \return true on success, false on failure
     */
    UHD_API bool set_thread_priority_safe(
        float priority = default_thread_priority,
        bool realtime = true
    );

    /*!
     * Set the thread name on the given boost thread.
     * \param thread pointer to a boost thread
     * \param name thread name with maximum length of 16 characters
     */
    UHD_API void set_thread_name(
            boost::thread *thread,
            const std::string &name
    );

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_THREAD_HPP */
