//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP
#define INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP

#include <vector>
#include <string>
#include <cstddef>

static constexpr size_t FPGPIO_MASTER_RADIO = 0;
static constexpr size_t TOTAL_RADIO_PORTS = 4;
static constexpr double AD9371_RX_MIN_BANDWIDTH = 20.0e6;  // HZ
static constexpr double AD9371_RX_MAX_BANDWIDTH = 100.0e6; // HZ

static constexpr double AD9371_TX_MIN_BANDWIDTH = 20.0e6;  // HZ
static constexpr double AD9371_TX_MAX_BANDWIDTH = 100.0e6; // HZ

static constexpr double AD9371_MIN_FREQ = 300.0e6; // Hz
static constexpr double AD9371_MAX_FREQ = 6.0e9; // Hz

static constexpr double ADF4351_MIN_FREQ = 35.0e6;
static constexpr double ADF4351_MAX_FREQ = 4.4e9;

static constexpr double MAGNESIUM_RADIO_RATE = 125e6; // Hz
static constexpr double MAGNESIUM_MIN_FREQ = 1e6; // Hz
static constexpr double MAGNESIUM_MAX_FREQ = 6e9; // Hz

static constexpr double MAGNESIUM_LOWBAND_FREQ = 300e6;

static constexpr double AD9371_MIN_RX_GAIN = 0.0; // dB
static constexpr double AD9371_MAX_RX_GAIN = 30.0; // dB
static constexpr double AD9371_RX_GAIN_STEP = 0.5;
static constexpr double DSA_MIN_GAIN = 0; // dB
static constexpr double DSA_MAX_GAIN = 31.5; // dB
static constexpr double DSA_GAIN_STEP = 0.5; // db
static constexpr double AMP_MIN_GAIN = 0; // dB
static constexpr double AMP_MAX_GAIN = 10; // dB
static constexpr double AMP_GAIN_STEP = 10;// dB
static constexpr double AD9371_MIN_TX_GAIN = 0.0; // dB
static constexpr double AD9371_MAX_TX_GAIN = 41.95; // dB
static constexpr double AD9371_TX_GAIN_STEP = 0.05;
static constexpr double ALL_RX_MIN_GAIN = 0.0;
static constexpr double ALL_RX_MAX_GAIN = 75.0;
static constexpr double ALL_RX_GAIN_STEP = 0.5;
static constexpr double ALL_TX_MIN_GAIN = 0.0;
static constexpr double ALL_TX_MAX_GAIN = 65.0;
static constexpr double ALL_TX_GAIN_STEP = 0.5;

static const std::vector<std::string> MAGNESIUM_RX_ANTENNAS = {
    "TX/RX", "RX2", "CAL", "LOCAL"
};

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

// Note: MAGNESIUM_NUM_CHANS is independent of the number of chans per
// RFNoC block. TODO: When we go to one radio per dboard, this comment can
// be deleted.
static constexpr size_t MAGNESIUM_NUM_CHANS = 2;
static constexpr double MAGNESIUM_RX_IF_FREQ = 2.44e9;
static constexpr double MAGNESIUM_TX_IF_FREQ = 1.95e9;

#endif /* INCLUDED_LIBUHD_MAGNESIUM_CONSTANTS_HPP */
