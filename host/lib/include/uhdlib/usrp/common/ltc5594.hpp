//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/time_spec.hpp>
#include <functional>
#include <memory>

//! Control interface for an LTC5594 I/Q Demodulator
class ltc5594_iface
{
public:
    using sptr = std::shared_ptr<ltc5594_iface>;

    virtual ~ltc5594_iface() = default;

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint8_t, uint8_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint8_t(uint8_t)>;

    //! Factory
    //
    // \param write SPI write function object
    // \param read SPI read function object
    // \param unique_id Unique identifier for logging purposes
    static sptr make(
        write_fn_t&& poke16, read_fn_t&& peek16, const std::string& unique_id);

    //! Save state to chip
    virtual void commit() = 0;

    //! Read back register value from chip
    virtual void update_field(const std::string& field) = 0;

    //! Performs a reset of the LTC5594 by using the software reset register
    virtual void reset() = 0;

    //! Sets the Single-Ended LO Matching
    //
    // \param lo_freq The LO frequency in Hz
    virtual void set_lo_matching(const double lo_freq) = 0;

    //! Enables/disables the SDO readback mode
    //
    // \param enable True to enable, False to disable
    virtual void enable_sdo_readback(const bool enable) = 0;

    //! Sets the phase error adjustment
    //
    // \param phase_error_adj The phase error adjustment in degrees
    virtual void set_phase_error_adj(const double phase_error_adj) = 0;

    //! Sets the DC offset adjustment
    //
    // \param dc_off_i_adj The DC offset adjustment i component
    // \param dc_off_q_adj The DC offset adjustment q component
    virtual void set_dc_offset_adj(
        const double dc_off_i_adj, const double dc_off_q_adj) = 0;

    //! Sets the gain error adjustment
    //
    // \param gain_err_adj The gain error adjustment value
    virtual void set_gain_err_adj(const double gain_err_adj) = 0;
};
//! Used for LTC5594 LO matching
struct ltc5594_lo_matching_t
{
    double min_band_freq;
    double max_band_freq;
    size_t band;
    size_t cf1;
    size_t lf1;
    size_t cf2;
};

// Turn clang formatting off for a well readable table. This is table 2
// from https://www.analog.com/media/en/technical-documentation/data-sheets/LTC5594.pdf
// clang-format off
static const std::vector<ltc5594_lo_matching_t> lo_matching_table = {
// | min_band_freq | max_band_freq | band | cf1 | lf1 | cf2 | 
    {       300e6,         339e6,       0,    31,    3,   31 },
    {       339e6,         398e6,       0,    21,    3,   24 },
    {       398e6,         419e6,       0,    14,    3,   23 },
    {       419e6,         556e6,       0,    17,    2,   31 },
    {       556e6,         625e6,       0,    10,    2,   23 },
    {       625e6,         801e6,       0,    15,    1,   31 },
    {       801e6,         831e6,       0,    14,    1,   27 },
    {       831e6,        1046e6,       0,     8,    1,   21 },
    {      1046e6,        1242e6,       1,    31,    3,   31 },
    {      1242e6,        1411e6,       1,    21,    3,   28 },
    {      1411e6,        1696e6,       1,    17,    2,   26 },
    {      1696e6,        2070e6,       1,    15,    1,   31 },
    {           0,             0,       1,     8,    3,    3 }, // Default
    {      2070e6,        2470e6,       1,     8,    1,   21 },
    {      2470e6,        2980e6,       1,     2,    1,   10 },
    {      2980e6,        3500e6,       1,     1,    0,   19 },
    {      3500e6,        9000e6,       1,     0,    0,    0 }
};
// Turn clang-format back on
// clang-format on
