//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <uhd/config.hpp>
#include <cstdint>

namespace uhd { namespace usrp { namespace hbx {

struct UHD_API hbx_lo_gain_map_item_t
{
    double start_freq; // Start frequency of the range
    double stop_freq; // Stop frequency of the range
    double lo_power; // LO power setting
    double lo_gain; // LO gain setting
};

}}} // namespace uhd::usrp::hbx
