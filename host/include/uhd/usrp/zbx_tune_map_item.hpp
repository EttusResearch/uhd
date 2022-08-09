//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <uhd/config.hpp>
#include <cstring>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace zbx {

/*!
 * Describes if the LO for mixing to another frequency is injected on the high side or on
 * the low side of the signal.
 */
enum lo_inj_side_t : int { HIGH = -1, LOW = 1, NONE = 0 };

/*!
 * This struct holds an item of the ZBX tune map. Usually, this a vector of these structs
 * would be used and depending on the requested tune frequency an item would be selected.
 * This information should not be changed after initialization.
 */

struct UHD_API zbx_tune_map_item_t
{
    double min_band_freq;
    double max_band_freq;
    uint8_t rf_fltr;
    uint8_t if1_fltr;
    uint8_t if2_fltr;
    lo_inj_side_t lo1_inj_side;
    lo_inj_side_t lo2_inj_side;
    double if1_freq_min;
    double if1_freq_max;
    double if2_freq_min;
    double if2_freq_max;

    bool operator==(const zbx_tune_map_item_t& other) const
    {
        return std::memcmp(this, &other, sizeof(zbx_tune_map_item_t)) == 0;
    }
};
}}} // namespace uhd::usrp::zbx
