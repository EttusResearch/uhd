//
// Copyright 2010,2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_PLATFORM_HPP
#define INCLUDED_UHD_UTILS_PLATFORM_HPP

#include <stdint.h>

namespace uhd {

    /* Returns the process ID of the current process */
    int32_t get_process_id();

    /* Returns a unique identifier for the current machine */
    uint32_t get_host_id();

    /* Get a unique identifier for the current machine and process */
    uint32_t get_process_hash();

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PLATFORM_HPP */
