//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_CLOCK_H
#define INCLUDED_UHD_USRP_CLOCK_H

#include <uhd/config.h>
#include <uhd/error.h>
#include <uhd/types/sensors.h>
#include <uhd/types/string_vector.h>

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/****************************************************************************
 * Public Datatypes for USRP clock
 ***************************************************************************/
struct uhd_usrp_clock;

//! A C-level interface for interacting with an Ettus Research clock device
/*!
 * See uhd::usrp_clock::multi_usrp_clock for more details.
 *
 * NOTE: Attempting to use a handle before passing it into uhd_usrp_clock_make()
 * will result in undefined behavior.
 */
typedef struct uhd_usrp_clock* uhd_usrp_clock_handle;

/****************************************************************************
 * Make / Free API calls
 ***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

//! Find all connected clock devices.
/*!
 * See uhd::device::find() for more details.
 */
UHD_API uhd_error uhd_usrp_clock_find(
    const char* args,
    uhd_string_vector_t *devices_out
);

//! Create a clock handle.
/*!
 * \param h The handle
 * \param args Device args (e.g. "addr=192.168.10.3")
 */
UHD_API uhd_error uhd_usrp_clock_make(
    uhd_usrp_clock_handle *h,
    const char *args
);

//! Safely destroy the clock object underlying the handle.
/*!
 * Note: After calling this, usage of h may cause segmentation faults.
 * However, multiple calling of uhd_usrp_free() is safe.
 */
UHD_API uhd_error uhd_usrp_clock_free(
    uhd_usrp_clock_handle *h
);

//! Get last error
UHD_API uhd_error uhd_usrp_clock_last_error(
    uhd_usrp_clock_handle h,
    char* error_out,
    size_t strbuffer_len
);

//! Get board information in a nice output
UHD_API uhd_error uhd_usrp_clock_get_pp_string(
    uhd_usrp_clock_handle h,
    char* pp_string_out,
    size_t strbuffer_len
);

//! Get number of boards
UHD_API uhd_error uhd_usrp_clock_get_num_boards(
    uhd_usrp_clock_handle h,
    size_t *num_boards_out
);

//! Get time
UHD_API uhd_error uhd_usrp_clock_get_time(
    uhd_usrp_clock_handle h,
    size_t board,
    uint32_t *clock_time_out
);

//! Get sensor
UHD_API uhd_error uhd_usrp_clock_get_sensor(
    uhd_usrp_clock_handle h,
    const char* name,
    size_t board,
    uhd_sensor_value_handle *sensor_value_out
);

//! Get sensor names
UHD_API uhd_error uhd_usrp_clock_get_sensor_names(
    uhd_usrp_clock_handle h,
    size_t board,
    uhd_string_vector_handle *sensor_names_out
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_USRP_CLOCK_H */
