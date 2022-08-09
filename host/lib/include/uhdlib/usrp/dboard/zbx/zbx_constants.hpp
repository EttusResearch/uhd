//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/zbx_tune_map_item.hpp>
#include <unordered_map>
#include <array>
#include <cstddef>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace zbx {

//! Which LO to address when peeking/poking
//  This must match the LO_SELECT values in gen_zbx_cpld_regs.py
enum class zbx_lo_t {
    TX0_LO1 = 0,
    TX0_LO2 = 1,
    TX1_LO1 = 2,
    TX1_LO2 = 3,
    RX0_LO1 = 4,
    RX0_LO2 = 5,
    RX1_LO1 = 6,
    RX1_LO2 = 7
};

static const std::map<zbx_lo_t, std::string> ZBX_LO_LOG_ID = {
    {zbx_lo_t::TX0_LO1, "ZBX TX0 LO1"},
    {zbx_lo_t::TX0_LO2, "ZBX TX0 LO2"},
    {zbx_lo_t::TX1_LO1, "ZBX TX1 LO1"},
    {zbx_lo_t::TX1_LO2, "ZBX TX1 LO2"},
    {zbx_lo_t::RX0_LO1, "ZBX RX0 LO1"},
    {zbx_lo_t::RX0_LO2, "ZBX RX0 LO2"},
    {zbx_lo_t::RX1_LO1, "ZBX RX1 LO1"},
    {zbx_lo_t::RX1_LO2, "ZBX RX1 LO2"}};

static constexpr std::array<zbx_lo_t, 8> ZBX_ALL_LO = {zbx_lo_t::TX0_LO1,
    zbx_lo_t::TX0_LO2,
    zbx_lo_t::TX1_LO1,
    zbx_lo_t::TX1_LO2,
    zbx_lo_t::RX0_LO1,
    zbx_lo_t::RX0_LO2,
    zbx_lo_t::RX1_LO1,
    zbx_lo_t::RX1_LO2};


/******************************************************************************
 * Important: When changing values here, check if that also requires updating
 * the manual (host/docs/zbx.dox). If it also requires changing the website or
 * other sales/marketing material, make sure to let the appropriate people know!
 *****************************************************************************/

enum class zbx_lo_source_t { internal, external };
static constexpr zbx_lo_source_t ZBX_DEFAULT_LO_SOURCE = zbx_lo_source_t::internal;

// The ZBX has a non-configurable analog bandwidth of 400 MHz. At lower frequency,
// the usable bandwidth may be smaller though. For those smaller bandwidths, see
// the tune maps.
static constexpr double ZBX_DEFAULT_BANDWIDTH = 400e6; // Hz

static constexpr double LMX2572_MAX_FREQ = 6.4e9; // Hz
// LMX2572 can go lower, but on the ZBX, the analog paths limit frequencies down
// to 3.2 GHz
static constexpr double LMX2572_MIN_FREQ         = 3.2e9; // Hz
static constexpr double LMX2572_DEFAULT_FREQ     = 4e9; // Hz
static constexpr uint32_t ZBX_LO_LOCK_TIMEOUT_MS = 20; // milliseconds
// This is the step size for the LO tuning relative to the PRC rate:
static constexpr int ZBX_RELATIVE_LO_STEP_SIZE = 6;

static constexpr double ZBX_MIN_FREQ     = 1e6; // Hz
static constexpr double ZBX_MAX_FREQ     = 8e9; // Hz
static constexpr double ZBX_DEFAULT_FREQ = 1e9; // Hz
static const uhd::freq_range_t ZBX_FREQ_RANGE(ZBX_MIN_FREQ, ZBX_MAX_FREQ);
static constexpr double ZBX_LOWBAND_FREQ = 3e9; // Hz

constexpr char HW_GAIN_STAGE[] = "hw";

static constexpr double RX_MIN_GAIN         = 0;
static constexpr double RX_MAX_GAIN         = 60;
static constexpr double RX_GAIN_STEP        = 1;
static constexpr double ZBX_DEFAULT_RX_GAIN = RX_MIN_GAIN;
static const uhd::gain_range_t ZBX_RX_GAIN_RANGE(RX_MIN_GAIN, RX_MAX_GAIN, RX_GAIN_STEP);
// Rx gain is limited to [0, 38] for frequency <= 500 MHz
static constexpr double RX_LOW_FREQ_MAX_GAIN        = 38;
static constexpr double RX_LOW_FREQ_MAX_GAIN_CUTOFF = 500e6; // Hz
static const uhd::gain_range_t ZBX_RX_LOW_FREQ_GAIN_RANGE(
    RX_MIN_GAIN, RX_LOW_FREQ_MAX_GAIN, RX_GAIN_STEP);
static constexpr double TX_MIN_GAIN         = 0;
static constexpr double TX_MAX_GAIN         = 60;
static constexpr double TX_GAIN_STEP        = 1;
static constexpr double ZBX_DEFAULT_TX_GAIN = TX_MIN_GAIN;
static const uhd::gain_range_t ZBX_TX_GAIN_RANGE(TX_MIN_GAIN, TX_MAX_GAIN, TX_GAIN_STEP);

static constexpr char ZBX_GAIN_PROFILE_DEFAULT[]        = "default";
static constexpr char ZBX_GAIN_PROFILE_MANUAL[]         = "manual";
static constexpr char ZBX_GAIN_PROFILE_CPLD[]           = "table";
static constexpr char ZBX_GAIN_PROFILE_CPLD_NOATR[]     = "table_noatr";
static const std::vector<std::string> ZBX_GAIN_PROFILES = {ZBX_GAIN_PROFILE_DEFAULT,
    ZBX_GAIN_PROFILE_MANUAL,
    ZBX_GAIN_PROFILE_CPLD,
    ZBX_GAIN_PROFILE_CPLD_NOATR};

// Maximum attenuation of the TX DSAs
static constexpr uint8_t ZBX_TX_DSA_MAX_ATT = 31;
// Maximum attenuation of the RX DSAs
static constexpr uint8_t ZBX_RX_DSA_MAX_ATT = 15;

static constexpr char ZBX_GAIN_STAGE_DSA1[]  = "DSA1";
static constexpr char ZBX_GAIN_STAGE_DSA2[]  = "DSA2";
static constexpr char ZBX_GAIN_STAGE_DSA3A[] = "DSA3A";
static constexpr char ZBX_GAIN_STAGE_DSA3B[] = "DSA3B";
static constexpr char ZBX_GAIN_STAGE_AMP[]   = "AMP";
static constexpr char ZBX_GAIN_STAGE_ALL[]   = "all";
// Not technically a gain stage, but we'll keep it
static constexpr char ZBX_GAIN_STAGE_TABLE[] = "TABLE";

static const std::vector<std::string> ZBX_RX_GAIN_STAGES = {
    ZBX_GAIN_STAGE_DSA1, ZBX_GAIN_STAGE_DSA2, ZBX_GAIN_STAGE_DSA3A, ZBX_GAIN_STAGE_DSA3B};

static const std::vector<std::string> ZBX_TX_GAIN_STAGES = {
    ZBX_GAIN_STAGE_DSA1, ZBX_GAIN_STAGE_DSA2, ZBX_GAIN_STAGE_AMP};

enum class tx_amp { BYPASS = 0, LOWBAND = 1, HIGHBAND = 2 };

static constexpr double ZBX_TX_BYPASS_GAIN   = 0.0;
static constexpr double ZBX_TX_LOWBAND_GAIN  = 14.0;
static constexpr double ZBX_TX_HIGHBAND_GAIN = 21.0;

// The amplifier gain varies wildly across frequency, temperature.... but we
// need some kind of mapping for querying/setting individual gain stages by
// dB value.
static const std::map<tx_amp, double> ZBX_TX_AMP_GAIN_MAP = {
    {tx_amp::BYPASS, ZBX_TX_BYPASS_GAIN},
    {tx_amp::LOWBAND, ZBX_TX_LOWBAND_GAIN},
    {tx_amp::HIGHBAND, ZBX_TX_HIGHBAND_GAIN}};
static const std::map<double, tx_amp> ZBX_TX_GAIN_AMP_MAP = {
    {ZBX_TX_BYPASS_GAIN, tx_amp::BYPASS},
    {ZBX_TX_LOWBAND_GAIN, tx_amp::LOWBAND},
    {ZBX_TX_HIGHBAND_GAIN, tx_amp::HIGHBAND}};


/*** Antenna-related constants ***********************************************/
// TX and RX SMA connectors on the front panel
constexpr char ANTENNA_TXRX[] = "TX/RX0";
constexpr char ANTENNA_RX[]   = "RX1";
// Internal "antenna" ports
constexpr char ANTENNA_CAL_LOOPBACK[] = "CAL_LOOPBACK";
constexpr char ANTENNA_TERMINATION[]  = "TERMINATION"; // Only RX path
// Default antennas (which are selected at init)
constexpr auto DEFAULT_TX_ANTENNA = ANTENNA_TXRX;
constexpr auto DEFAULT_RX_ANTENNA = ANTENNA_RX;
// Helper lists
static const std::vector<std::string> RX_ANTENNAS = {
    ANTENNA_TXRX, ANTENNA_RX, ANTENNA_CAL_LOOPBACK, ANTENNA_TERMINATION};
static const std::vector<std::string> TX_ANTENNAS = {ANTENNA_TXRX, ANTENNA_CAL_LOOPBACK};
// For branding purposes, ZBX changed the antenna names around. For existing
// software, we still accept the old antenna names, but map them to the new ones
static const std::unordered_map<std::string, std::string> TX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}};
static const std::unordered_map<std::string, std::string> RX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}, {"RX2", ANTENNA_RX}};

/*** LO-related constants ****************************************************/
//! Low-band LO
static constexpr char ZBX_LO1[] = "LO1";
//! LO at 2nd mixer
static constexpr char ZBX_LO2[] = "LO2";

static constexpr char RFDC_NCO[] = "rfdc";

static const std::vector<std::string> ZBX_LOS = {ZBX_LO1, ZBX_LO2, RFDC_NCO};

static constexpr size_t ZBX_NUM_CHANS = 2;
static constexpr std::array<size_t, 2> ZBX_CHANNELS{0, 1};

static constexpr double ZBX_MIX1_MN_THRESHOLD = 4e9;

// These are addresses for the various table-based registers
static constexpr uint32_t ATR_ADDR_0X = 0;
static constexpr uint32_t ATR_ADDR_RX = 1;
static constexpr uint32_t ATR_ADDR_TX = 2;
static constexpr uint32_t ATR_ADDR_XX = 3; // Full-duplex
// Helper for looping
static constexpr std::array<uint32_t, 4> ATR_ADDRS{0, 1, 2, 3};

// Turn clang-formatting off so it doesn't compress these tables into a mess.
// clang-format off
static const std::vector<zbx_tune_map_item_t> rx_tune_map = {
//  | min_band_freq | max_band_freq | rf_fltr | if1_fltr | if2_fltr | lo1_inj_side | lo2_inj_side | if1_freq_min | if1_freq_max | if2_freq_min | if2_freq_max |
    {      1e6,          200e6,           1,        1,         2,       HIGH,         HIGH,          4100e6,        4100e6,        1850e6,        1850e6    },
    {    200e6,          400e6,           1,        1,         2,       HIGH,         HIGH,          4100e6,        4100e6,        1850e6,        1850e6    },
    {    400e6,          500e6,           1,        1,         2,       HIGH,         HIGH,          4100e6,        4100e6,        1850e6,        1850e6    },
    {    500e6,          900e6,           1,        1,         2,       HIGH,         HIGH,          4100e6,        4100e6,        1850e6,        1850e6    },
    {    900e6,         1800e6,           1,        1,         2,       HIGH,         HIGH,          4100e6,        4100e6,        2150e6,        2150e6    },
    {   1800e6,         2300e6,           2,        1,         1,       HIGH,         HIGH,          4100e6,        4100e6,        1060e6,        1060e6    },
    {   2300e6,         2700e6,           3,        1,         1,       HIGH,         HIGH,          4100e6,        3700e6,        1060e6,        1060e6    },
    {   2700e6,         3000e6,           3,        4,         2,        LOW,          LOW,          7000e6,        7100e6,        2050e6,        2080e6    },
    {   3000e6,         4200e6,           0,        1,         2,       NONE,         HIGH,               0,             0,        1850e6,        1850e6    },
    {   4200e6,         4500e6,           0,        2,         2,       NONE,         HIGH,               0,             0,        1850e6,        1850e6    },
    {   4500e6,         4700e6,           0,        2,         1,       NONE,         HIGH,               0,             0,        1060e6,        1060e6    },
    {   4700e6,         5300e6,           0,        2,         1,       NONE,         HIGH,               0,             0,        1060e6,        1060e6    },
    {   5300e6,         5600e6,           0,        2,         1,       NONE,          LOW,               0,             0,        1060e6,        1060e6    },
    {   5600e6,         6800e6,           0,        3,         1,       NONE,          LOW,               0,             0,        1060e6,        1060e6    },
    {   6800e6,         7400e6,           0,        4,         1,       NONE,          LOW,               0,             0,        1060e6,        1060e6    },
    {   7400e6,         8000e6,           0,        4,         2,       NONE,          LOW,               0,             0,        1850e6,        1850e6    },
};

static const std::vector<zbx_tune_map_item_t> tx_tune_map = {
//  | min_band_freq | max_band_freq | rf_fltr | if1_fltr | if2_fltr | lo1_inj_side | lo2_inj_side | if1_freq_min | if1_freq_max | if2_freq_min | if2_freq_max |
    {      1e6,          200e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {    200e6,          300e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {    300e6,          400e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {    400e6,          600e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {    600e6,          800e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {    800e6,         1300e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {   1300e6,         1800e6,           1,        2,         1,          HIGH,           LOW,       4600e6,        4600e6,        1060e6,        1060e6    },
    {   1800e6,         2300e6,           2,        1,         1,          HIGH,          HIGH,       4100e6,        4100e6,        1060e6,        1060e6    },
    {   2300e6,         2700e6,           3,        1,         2,          HIGH,          HIGH,       3700e6,        3700e6,        2070e6,        2200e6    },
    {   2700e6,         3000e6,           3,        5,         2,           LOW,           LOW,       6800e6,        7100e6,        2000e6,        2000e6    },
    {   3000e6,         4030e6,           0,        1,         2,          NONE,          HIGH,            0,             0,        2050e6,        2370e6    },
    {   4030e6,         4500e6,           0,        1,         1,          NONE,          HIGH,            0,             0,        1060e6,        1060e6    },
    {   4500e6,         4900e6,           0,        2,         1,          NONE,           LOW,            0,             0,        1060e6,        1060e6    },
    {   4900e6,         5100e6,           0,        2,         1,          NONE,           LOW,            0,             0,        1060e6,        1060e6    },
    {   5100e6,         5700e6,           0,        3,         2,          NONE,           LOW,            0,             0,        1900e6,        2300e6    },
    {   5700e6,         6100e6,           0,        4,         2,          NONE,           LOW,            0,             0,        2300e6,        2500e6    },
    {   6100e6,         6400e6,           0,        4,         2,          NONE,           LOW,            0,             0,        2400e6,        2500e6    },
    {   6400e6,         7000e6,           0,        5,         2,          NONE,           LOW,            0,             0,        1900e6,        1950e6    },
    {   7000e6,         7400e6,           0,        6,         1,          NONE,           LOW,            0,             0,        1060e6,        1060e6    },
    {   7400e6,         8000e6,           0,        6,         2,          NONE,           LOW,            0,             0,        1950e6,        2050e6    },
};

// Turn clang-format back on just for posterity
// clang-format on

}}} // namespace uhd::usrp::zbx


namespace uhd { namespace usrp { namespace zbx {
// << Operator overload for expert's node printing (zbx_lo_source_t property)
// Any added expert nodes of type enum class will have to define this
std::ostream& operator<<(
    std::ostream& os, const ::uhd::usrp::zbx::zbx_lo_source_t& lo_source);
std::ostream& operator<<(
    std::ostream& os, const std::vector<::uhd::usrp::zbx::zbx_tune_map_item_t>& tune_map);
}}} // namespace uhd::usrp::zbx
