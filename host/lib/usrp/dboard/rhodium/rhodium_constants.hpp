//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP
#define INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP

#include <array>
#include <cstddef>
#include <string>
#include <vector>

static constexpr size_t NUM_RHODIUM_RADIO_RATES = 3;

static constexpr std::array<double, NUM_RHODIUM_RADIO_RATES> RHODIUM_RADIO_RATES = {
    200e6, 245.76e6, 250e6};

static constexpr double RHODIUM_MIN_FREQ = 1e6; // Hz
static constexpr double RHODIUM_MAX_FREQ = 6e9; // Hz

static constexpr double RHODIUM_LO1_MIN_FREQ = 450e6; // Hz
static constexpr double RHODIUM_LO1_MAX_FREQ = 6e9; // Hz
static constexpr double RHODIUM_LO1_REF_FREQ = 122.88e6; // Hz

static constexpr double RHODIUM_LO_0_9_GHZ_LPF_THRESHOLD_FREQ  = 0.975e9; // Hz
static constexpr double RHODIUM_LO_2_25_GHZ_LPF_THRESHOLD_FREQ = 2.3e9; // Hz

static constexpr double RX_MIN_GAIN  = 0.0;
static constexpr double RX_MAX_GAIN  = 60.0;
static constexpr double RX_GAIN_STEP = 1.0;
static constexpr double TX_MIN_GAIN  = 0.0;
static constexpr double TX_MAX_GAIN  = 60.0;
static constexpr double TX_GAIN_STEP = 1.0;

static constexpr double LO_MIN_GAIN  = 0.0;
static constexpr double LO_MAX_GAIN  = 30.0;
static constexpr double LO_GAIN_STEP = 1.0;

static constexpr double LO_MIN_POWER  = 0.0;
static constexpr double LO_MAX_POWER  = 63.0;
static constexpr double LO_POWER_STEP = 1.0;

static constexpr double RHODIUM_DEFAULT_BANDWIDTH = 250e6; // Hz

static const std::vector<std::string> RHODIUM_RX_ANTENNAS = {
    "TX/RX", "RX2", "CAL", "TERM"};

static const std::vector<std::string> RHODIUM_TX_ANTENNAS = {"TX/RX", "CAL", "TERM"};

// These names are taken from radio_rhodium.xml
static constexpr char SPUR_DODGING_PROP_NAME[]                = "spur_dodging";
static constexpr char SPUR_DODGING_THRESHOLD_PROP_NAME[]      = "spur_dodging_threshold";
static constexpr char HIGHBAND_SPUR_REDUCTION_PROP_NAME[]     = "highband_spur_reduction";
static constexpr char RHODIUM_DEFAULT_SPUR_DOGING_MODE[]      = "disabled";
static constexpr double RHODIUM_DEFAULT_SPUR_DOGING_THRESHOLD = 2e6;
static constexpr char RHODIUM_DEFAULT_HB_SPUR_REDUCTION_MODE[] = "disabled";


static constexpr char RHODIUM_FPGPIO_BANK[] = "FP0";
static constexpr uint32_t RHODIUM_GPIO_MASK = 0x1F;
static constexpr uint32_t SW10_GPIO_MASK    = 0x3;
static constexpr uint32_t LED_GPIO_MASK     = 0x1C;

static constexpr uint32_t SW10_FROMTXLOWBAND  = 0x0;
static constexpr uint32_t SW10_FROMTXHIGHBAND = 0x1;
static constexpr uint32_t SW10_ISOLATION      = 0x2;
static constexpr uint32_t SW10_TORX           = 0x3;

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

static constexpr char RHODIUM_FE_NAME[] = "Rhodium";

static constexpr int NUM_LO_OUTPUT_PORT_NAMES = 4;
static constexpr std::array<const char*, NUM_LO_OUTPUT_PORT_NAMES> LO_OUTPUT_PORT_NAMES =
    {"LO_OUT_0", "LO_OUT_1", "LO_OUT_2", "LO_OUT_3"};

static constexpr size_t RHODIUM_NUM_CHANS = 1;

namespace n320_regs {

static constexpr uint32_t PERIPH_BASE       = 0x80000;
static constexpr uint32_t PERIPH_REG_OFFSET = 8;

// db_control registers
static constexpr uint32_t SR_MISC_OUTS = PERIPH_BASE + 160 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_SPI       = PERIPH_BASE + 168 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_LEDS      = PERIPH_BASE + 176 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_FP_GPIO   = PERIPH_BASE + 184 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_DB_GPIO   = PERIPH_BASE + 192 * PERIPH_REG_OFFSET;

static constexpr uint32_t RB_MISC_IO = PERIPH_BASE + 16 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_SPI     = PERIPH_BASE + 17 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_LEDS    = PERIPH_BASE + 18 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_DB_GPIO = PERIPH_BASE + 19 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_FP_GPIO = PERIPH_BASE + 20 * PERIPH_REG_OFFSET;

//! Delta between frontend offsets for channel 0 and 1
constexpr uint32_t SR_TX_FE_BASE = PERIPH_BASE + 208 * PERIPH_REG_OFFSET;
constexpr uint32_t SR_RX_FE_BASE = PERIPH_BASE + 224 * PERIPH_REG_OFFSET;

} // namespace n320_regs

#endif /* INCLUDED_LIBUHD_RHODIUM_CONSTANTS_HPP */
