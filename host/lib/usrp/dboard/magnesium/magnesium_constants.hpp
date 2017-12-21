//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP
#define INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP

#include <vector>
#include <string>
#include <cstddef>

static const size_t FPGPIO_MASTER_RADIO = 0;

static const double AD9371_MIN_FREQ = 300.0e6; // Hz
static const double AD9371_MAX_FREQ = 6.0e9; // Hz

static const double ADF4351_MIN_FREQ = 35.0e6;
static const double ADF4351_MAX_FREQ = 4.4e9;

static const double MAGNESIUM_RADIO_RATE = 125e6; // Hz
static const double MAGNESIUM_MIN_FREQ = 1e6; // Hz
static const double MAGNESIUM_MAX_FREQ = 6e9; // Hz

static const double MAGNESIUM_LOWBAND_FREQ = 300e6;

static const double MAGNESIUM_RX_BAND1_MIN_FREQ = MAGNESIUM_LOWBAND_FREQ;
static const double MAGNESIUM_RX_BAND2_MIN_FREQ = 600e6;
static const double MAGNESIUM_RX_BAND3_MIN_FREQ = 1050e6;
static const double MAGNESIUM_RX_BAND4_MIN_FREQ = 1600e6;
static const double MAGNESIUM_RX_BAND5_MIN_FREQ = 2100e6;
static const double MAGNESIUM_RX_BAND6_MIN_FREQ = 2700e6;

static const double MAGNESIUM_TX_BAND1_MIN_FREQ = MAGNESIUM_LOWBAND_FREQ;
static const double MAGNESIUM_TX_BAND2_MIN_FREQ = 800e6;
static const double MAGNESIUM_TX_BAND3_MIN_FREQ = 1700e6;
static const double MAGNESIUM_TX_BAND4_MIN_FREQ = 3400e6;

static const double AD9371_MIN_RX_GAIN = 0.0; // dB
static const double AD9371_MAX_RX_GAIN = 30.0; // dB
static const double AD9371_RX_GAIN_STEP = 0.5;
static const double DSA_MIN_GAIN = 0; // dB
static const double DSA_MAX_GAIN = 31.5; // dB
static const double DSA_GAIN_STEP = 0.5; // db
static const double AD9371_MIN_TX_GAIN = 0.0; // dB
static const double AD9371_MAX_TX_GAIN = 41.95; // dB
static const double AD9371_TX_GAIN_STEP = 0.05;
static const double ALL_RX_MIN_GAIN = 0.0;
static const double ALL_RX_MAX_GAIN = 75.0;
static const double ALL_RX_GAIN_STEP = 0.5;
static const double ALL_TX_MIN_GAIN = 0.0;
static const double ALL_TX_MAX_GAIN = 65.0;
static const double ALL_TX_GAIN_STEP = 0.5;
static const double MAGNESIUM_CENTER_FREQ = 2.5e9; // Hz
static const std::vector<std::string> MAGNESIUM_RX_ANTENNAS = {
    "TX/RX", "RX2", "CAL", "LOCAL"
};
//! AD9371 LO (for direct conversion)
static const char* MAGNESIUM_LO1 = "rfic";
//! Low-band LO (for IF conversion)
static const char* MAGNESIUM_LO2 = "lowband";

static const double MAGNESIUM_DEFAULT_BANDWIDTH = 40e6; // Hz TODO: fix
// Note: MAGNESIUM_NUM_CHANS is independent of the number of chans per
// RFNoC block. TODO: When we go to one radio per dboard, this comment can
// be deleted.
static const size_t MAGNESIUM_NUM_CHANS = 2;
static const double MAGNESIUM_RX_IF_FREQ = 2.44e9;
static const double MAGNESIUM_TX_IF_FREQ = 1.95e9;

#endif /* INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP */
