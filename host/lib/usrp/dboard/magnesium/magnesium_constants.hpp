//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP
#define INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP

#include <uhd/types/ranges.hpp>
#include <cstddef>
#include <string>
#include <vector>

static constexpr size_t TOTAL_RADIO_PORTS       = 4;
static constexpr double AD9371_RX_MIN_BANDWIDTH = 20.0e6; // HZ
static constexpr double AD9371_RX_MAX_BANDWIDTH = 100.0e6; // HZ

static constexpr double AD9371_TX_MIN_BANDWIDTH = 20.0e6; // HZ
static constexpr double AD9371_TX_MAX_BANDWIDTH = 100.0e6; // HZ

static constexpr double AD9371_MIN_FREQ = 300.0e6; // Hz
static constexpr double AD9371_MAX_FREQ = 6.0e9; // Hz

static constexpr size_t AD9371_RX_MAX_FIR_TAPS = 48; // Coefficients
static constexpr size_t AD9371_TX_MAX_FIR_TAPS = 32; // Coefficients

static constexpr double ADF4351_MIN_FREQ = 35.0e6;
static constexpr double ADF4351_MAX_FREQ = 4.4e9;

static const std::vector<double> MAGNESIUM_RADIO_RATES = {122.88e6, 125e6, 153.6e6};
static constexpr double MAGNESIUM_RADIO_RATE           = 125e6; // Hz
static constexpr double MAGNESIUM_MIN_FREQ             = 1e6; // Hz
static constexpr double MAGNESIUM_MAX_FREQ             = 6e9; // Hz

static constexpr double MAGNESIUM_LOWBAND_FREQ = 300e6;

static constexpr double AD9371_MIN_RX_GAIN  = 0.0; // dB
static constexpr double AD9371_MAX_RX_GAIN  = 30.0; // dB
static constexpr double AD9371_RX_GAIN_STEP = 0.5;
static constexpr double DSA_MIN_GAIN        = 0; // dB
static constexpr double DSA_MAX_GAIN        = 31.5; // dB
static constexpr double DSA_GAIN_STEP       = 0.5; // db
static constexpr double AMP_MIN_GAIN        = 0; // dB
static constexpr double AMP_MAX_GAIN        = 10; // dB
static constexpr double AMP_GAIN_STEP       = 10; // dB
static constexpr double AD9371_MIN_TX_GAIN  = 0.0; // dB
static constexpr double AD9371_MAX_TX_GAIN  = 41.95; // dB
static constexpr double AD9371_TX_GAIN_STEP = 0.05;
static constexpr double ALL_RX_MIN_GAIN     = 0.0;
static constexpr double ALL_RX_MAX_GAIN     = 75.0;
static constexpr double ALL_RX_GAIN_STEP    = 0.5;
static constexpr double ALL_TX_MIN_GAIN     = 0.0;
static constexpr double ALL_TX_MAX_GAIN     = 65.0;
static constexpr double ALL_TX_GAIN_STEP    = 0.5;

static const uhd::freq_range_t MAGNESIUM_FREQ_RANGE(
    MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ);

static const std::vector<std::string> MAGNESIUM_RX_ANTENNAS = {
    "TX/RX", "RX2", "CAL", "LOCAL"};

//! AD9371 LO (for direct conversion)
static constexpr char MAGNESIUM_LO1[] = "rfic";
//! Low-band LO (for IF conversion)
static constexpr char MAGNESIUM_LO2[] = "lowband";
//! AD9371 attenuation
static constexpr char MAGNESIUM_GAIN1[] = "rfic";
//! DSA attenuation
static constexpr char MAGNESIUM_GAIN2[] = "dsa";
//! Amplifier gain
static constexpr char MAGNESIUM_AMP[] = "amp";

static constexpr char MAGNESIUM_FE_NAME[] = "Magnesium";

static constexpr char MAGNESIUM_DEFAULT_RX_ANTENNA[] = "RX2";
static constexpr char MAGNESIUM_DEFAULT_TX_ANTENNA[] = "TX/RX";

static constexpr char MAGNESIUM_FPGPIO_BANK[] = "FP0";

// Note: MAGNESIUM_NUM_CHANS is independent of the number of chans per
// RFNoC block. TODO: When we go to one radio per dboard, this comment can
// be deleted.
static constexpr size_t MAGNESIUM_NUM_CHANS  = 2;
static constexpr double MAGNESIUM_RX_IF_FREQ = 2.4418e9;
static constexpr double MAGNESIUM_TX_IF_FREQ = 1.95e9;

//! Max time we allow for a call to set_freq() to take
static constexpr size_t MAGNESIUM_TUNE_TIMEOUT = 15000; // milliseconds

//! Magnesium gain profile options
static const std::vector<std::string> MAGNESIUM_GP_OPTIONS = {"manual",
    "default",
    "default_rf_filter_bypass_always_on",
    "default_rf_filter_bypass_always_off"};

namespace n310_regs {

static constexpr uint32_t PERIPH_BASE = 0x80000;
// Space between registers
static constexpr uint32_t PERIPH_REG_OFFSET = 8;
// Space between channels on the same dboard
static constexpr uint32_t CHAN_REG_OFFSET = 0x100 * PERIPH_REG_OFFSET;

// db_control registers
static constexpr uint32_t SR_MISC_OUTS = PERIPH_BASE + 160 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_SPI       = PERIPH_BASE + 168 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_FP_GPIO   = PERIPH_BASE + 184 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_DB_GPIO   = PERIPH_BASE + 192 * PERIPH_REG_OFFSET;

static constexpr uint32_t RB_MISC_IO = PERIPH_BASE + 16 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_SPI     = PERIPH_BASE + 17 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_DB_GPIO = PERIPH_BASE + 19 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_FP_GPIO = PERIPH_BASE + 20 * PERIPH_REG_OFFSET;
} // namespace n310_regs

#endif /* INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP */
