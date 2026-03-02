//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <uhd/config.hpp>
#include <cstdint>

namespace uhd { namespace usrp { namespace hbx {

struct UHD_API hbx_rx_gain_map_item_t
{
    double start_freq; // Start frequency of the range
    double stop_freq; // Stop frequency of the range
    size_t gain_idx; // The index of the gain setting per frequency range
    uint8_t lf_dsa1; // Low-frequency DSA1 setting
    uint8_t lf_dsa2; // Low-frequency DSA2 setting
    uint8_t rf_dsa1; // RF DSA1 setting
    uint8_t admv_dsa; // ADMV compound DSA setting
};

struct UHD_API hbx_tx_gain_map_item_t
{
    double start_freq; // Start frequency of the range
    double stop_freq; // Stop frequency of the range
    size_t gain_idx; // The index of the gain setting per frequency range
    uint8_t rf_dsa1; // RF DSA1 setting
    uint8_t admv_dsa; // ADMV compound DSA 1-3 setting
};
}}} // namespace uhd::usrp::hbx
