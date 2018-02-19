//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_THREAD_PRIORITY_H
#define INCLUDED_UHD_UTILS_THREAD_PRIORITY_H

#include <uhd/config.h>
#include <uhd/error.h>

#ifdef __cplusplus
extern "C" {
#endif

static const float uhd_default_thread_priority = 0.5;

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
 * \return UHD error code
 */
UHD_API uhd_error uhd_set_thread_priority(
    float priority,
    bool realtime
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_UTILS_THREAD_PRIORITY_H */
