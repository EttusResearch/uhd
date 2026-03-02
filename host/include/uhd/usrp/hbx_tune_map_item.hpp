//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <uhd/config.hpp>
#include <uhdlib/usrp/common/x4xx_ch_modes.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

namespace uhd { namespace usrp { namespace hbx {
/*!
 * This struct holds an item of the HBX tune map. Depending on the requested tune
 * frequency, an item of this struct would be selected to configure the TX signal path
 * switches. This information should not be changed after initialization.
 */
struct UHD_API hbx_tune_map_item_t
{
    double start_tune_freq;
    double stop_tune_freq;
    uint8_t rf_band;
    uint8_t rf_band1_subband;
    uint8_t rf_filter_branch;
    std::vector<uint8_t> lo_filter_branch;
    size_t rfdc_mode;
    uint8_t admv_band;
    double lo_divider;

    // This checks if two instances of the tune item are equal
    bool operator==(const hbx_tune_map_item_t& other) const
    {
        return std::tie(start_tune_freq,
                   stop_tune_freq,
                   rf_band,
                   rf_band1_subband,
                   rf_filter_branch,
                   lo_filter_branch,
                   rfdc_mode,
                   admv_band,
                   lo_divider)
               == std::tie(other.start_tune_freq,
                   other.stop_tune_freq,
                   other.rf_band,
                   other.rf_band1_subband,
                   other.rf_filter_branch,
                   other.lo_filter_branch,
                   other.rfdc_mode,
                   other.admv_band,
                   other.lo_divider);
    }
};
}}} // namespace uhd::usrp::hbx
