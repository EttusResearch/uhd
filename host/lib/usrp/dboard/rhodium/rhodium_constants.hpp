//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP
#define INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP

#include <array>
#include <vector>
#include <string>
#include <cstddef>

static constexpr double RHODIUM_FREQ_COMPARE_EPSILON = 1e-5;

static constexpr size_t NUM_RHODIUM_RADIO_RATES = 3;

static constexpr std::array<double, NUM_RHODIUM_RADIO_RATES> RHODIUM_RADIO_RATES = {
    200e6, 245.76e6, 250e6};

static constexpr double RHODIUM_MIN_FREQ = 1e6; // Hz
static constexpr double RHODIUM_MAX_FREQ = 6e9; // Hz

static constexpr double RHODIUM_LO1_MIN_FREQ = 450e6; // Hz
static constexpr double RHODIUM_LO1_MAX_FREQ = 6e9; // Hz
static constexpr double RHODIUM_LO1_REF_FREQ = 122.88e6; // Hz

static constexpr double RHODIUM_LO_0_9_GHZ_LPF_THRESHOLD_FREQ = 0.975e9; // Hz
static constexpr double RHODIUM_LO_2_25_GHZ_LPF_THRESHOLD_FREQ = 2.3e9; // Hz

static constexpr double RHODIUM_LOWBAND_FREQ = 450e6; // Hz
static constexpr double RHODIUM_RX_IF_FREQ = 2.44e9; // Hz
static constexpr double RHODIUM_TX_IF_FREQ = 1.95e9; // Hz

static constexpr double RX_MIN_GAIN = 0.0;
static constexpr double RX_MAX_GAIN = 60.0;
static constexpr double RX_GAIN_STEP = 1.0;
static constexpr double TX_MIN_GAIN = 0.0;
static constexpr double TX_MAX_GAIN = 60.0;
static constexpr double TX_GAIN_STEP = 1.0;

static constexpr double LO_MIN_GAIN = 0.0;
static constexpr double LO_MAX_GAIN = 30.0;
static constexpr double LO_GAIN_STEP = 1.0;

static constexpr double LO_MIN_POWER = 0.0;
static constexpr double LO_MAX_POWER = 63.0;
static constexpr double LO_POWER_STEP = 1.0;

static const std::vector<std::string> RHODIUM_RX_ANTENNAS = {
    "TX/RX", "RX2", "CAL", "TERM"
};

static const std::vector<std::string> RHODIUM_TX_ANTENNAS = {
    "TX/RX", "CAL", "TERM"
};

// These names are taken from radio_rhodium.xml
static constexpr char SPUR_DODGING_ARG_NAME[]            = "spur_dodging";
static constexpr char SPUR_DODGING_THRESHOLD_ARG_NAME[]  = "spur_dodging_threshold";
static constexpr char HIGHBAND_SPUR_REDUCTION_ARG_NAME[] = "highband_spur_reduction";

static constexpr uint32_t RHODIUM_GPIO_MASK = 0x1F;
static constexpr uint32_t SW10_GPIO_MASK = 0x3;
static constexpr uint32_t LED_GPIO_MASK = 0x1C;

static constexpr uint32_t SW10_FROMTXLOWBAND = 0x0;
static constexpr uint32_t SW10_FROMTXHIGHBAND = 0x1;
static constexpr uint32_t SW10_ISOLATION = 0x2;
static constexpr uint32_t SW10_TORX = 0x3;

static constexpr uint32_t LED_RX  = 0x04;
static constexpr uint32_t LED_RX2 = 0x08;
static constexpr uint32_t LED_TX  = 0x10;

//! Main LO
static constexpr char RHODIUM_LO1[] = "lo1";
//! Low-band LO (for IF conversion)
static constexpr char RHODIUM_LO2[] = "lowband";
//! DSA attenuation
static constexpr char RHODIUM_GAIN[] = "gain_table";
//! LO DSA attenuation
static constexpr char RHODIUM_LO_GAIN[] = "dsa";
//! LO output power
static constexpr char RHODIUM_LO_POWER[] = "lo";

static constexpr int NUM_LO_OUTPUT_PORT_NAMES = 4;

static constexpr std::array<const char*, NUM_LO_OUTPUT_PORT_NAMES> LO_OUTPUT_PORT_NAMES = {
    "LO_OUT_0",
    "LO_OUT_1",
    "LO_OUT_2",
    "LO_OUT_3"
};

static constexpr size_t RHODIUM_NUM_CHANS = 1;

#endif /* INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP */
