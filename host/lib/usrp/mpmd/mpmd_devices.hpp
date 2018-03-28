//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#ifndef INCLUDED_MPMD_DEVICES_HPP
#define INCLUDED_MPMD_DEVICES_HPP

#include <vector>
#include <string>

static constexpr char MPM_CATCHALL_DEVICE_TYPE[] = "mpm";

// List all MPM devices here by their 'type' key. Note: Do not use these values
// to make decisions. This vector is for filtering purposes.
static const std::vector<std::string> MPM_DEVICE_TYPES = {
    MPM_CATCHALL_DEVICE_TYPE,
    "n3xx"
};

#endif /* INCLUDED_MPMD_DEVICES_HPP */
