/*
 * Copyright 2015 Ettus Research
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_UHD_TYPES_DEVICE_ADDRS_H
#define INCLUDED_UHD_TYPES_DEVICE_ADDRS_H

#include <uhd/config.h>
#include <uhd/error.h>

#include <stdlib.h>

#ifdef __cplusplus
#include <uhd/types/device_addr.hpp>
#include <string>

struct uhd_device_addrs_t {
    uhd::device_addrs_t device_addrs_cpp;
    std::string last_error;
};

extern "C" {
#else
struct uhd_device_addrs_t;
#endif

//! C-level interface for interacting with a list of device addresses
/*!
 * See uhd::device_addrs_t for more information.
 */
typedef struct uhd_device_addrs_t* uhd_device_addrs_handle;

//! Instantiate a device_addrs handle.
UHD_API uhd_error uhd_device_addrs_make(
    uhd_device_addrs_handle *h
);

//! Safely destroy a device_addrs handle.
UHD_API uhd_error uhd_device_addrs_free(
    uhd_device_addrs_handle *h
);

//! Add a device address to the list in string form
UHD_API uhd_error uhd_device_addrs_push_back(
    uhd_device_addrs_handle h,
    const char* value
);

//! Get the device information (in string form) at the given index
UHD_API uhd_error uhd_device_addrs_at(
    uhd_device_addrs_handle h,
    size_t index,
    char* value_out,
    size_t strbuffer_len
);

//! Get the number of device addresses in this list
UHD_API uhd_error uhd_device_addrs_size(
    uhd_device_addrs_handle h,
    size_t *size_out
);

//! Get the last error reported by the underlying object
UHD_API uhd_error uhd_device_addrs_last_error(
    uhd_device_addrs_handle h,
    char* error_out,
    size_t strbuffer_len
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_TYPES_DEVICE_ADDRS_H */
