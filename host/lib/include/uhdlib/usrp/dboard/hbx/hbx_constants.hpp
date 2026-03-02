//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/iq_dc_cal_coeffs.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/hbx_gain_map_item.hpp>
#include <uhd/usrp/hbx_lo_gain_map_item.hpp>
#include <uhd/usrp/hbx_tune_map_item.hpp>
#include <uhdlib/usrp/common/x4xx_ch_modes.hpp>
#include <unordered_map>
#include <array>
#include <cstddef>
#include <cstring>
#include <list>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace hbx {
using uhd::usrp::x400::ch_mode;

/******************************************************************************
 * Important: When changing values here, check if that also requires updating
 * the manual (host/docs/hbx.dox). If it also requires changing the website or
 * other sales/marketing material, make sure to let the appropriate people know!
 *****************************************************************************/

const static uint16_t HBX_PID                       = 0x4008;
static constexpr char HBX_LO[]                      = "HBX_LO";
static constexpr char GENERIC_LO[]                  = "all";
static const std::vector<std::string> LO_SOURCE_INT = {"internal"};
static const std::vector<std::string> LO_SOURCE_ALL = {"internal", "external"};
static constexpr char LO_PD[]                       = "lo_pd";

// The HBX has a non-configurable analog bandwidth of 1250 MHz. At lower frequency,
// or lower master clock rates, the usable bandwidth may be smaller though.
static constexpr double HBX_DEFAULT_BANDWIDTH = 1.25e9; // Hz

static constexpr double HBX_MIN_FREQ     = 10e6; // Hz
static constexpr double HBX_MAX_FREQ     = 20e9; // Hz
static constexpr double HBX_DEFAULT_FREQ = 1e9; // Hz
static const uhd::freq_range_t HBX_FREQ_RANGE(HBX_MIN_FREQ, HBX_MAX_FREQ);

/*** Antenna-related constants ***********************************************/
// TX and RX SMA connectors on the front panel
constexpr char ANTENNA_TXRX[] = "TX/RX0";
constexpr char ANTENNA_RX[]   = "RX1";
// Internal "antenna" ports
constexpr char ANTENNA_CAL_LOOPBACK[] = "CAL_LOOPBACK";
constexpr char ANTENNA_SYNC_INT[]     = "SYNC_INT";
constexpr char ANTENNA_TERMINATION[]  = "TERM"; // Only RX path
// Default antennas (which are selected at init)
constexpr auto DEFAULT_TX_ANTENNA = ANTENNA_TXRX;
constexpr auto DEFAULT_RX_ANTENNA = ANTENNA_RX;
// Helper lists
static const std::vector<std::string> RX_ANTENNAS = {ANTENNA_TXRX,
    ANTENNA_RX,
    ANTENNA_CAL_LOOPBACK,
    ANTENNA_TERMINATION,
    ANTENNA_SYNC_INT};

static const std::vector<std::string> TX_ANTENNAS = {ANTENNA_TXRX, ANTENNA_CAL_LOOPBACK};
// For branding purposes, HBX changed the antenna names around. For
// existing software, we still accept the old antenna names, but map them to the new ones
static const std::unordered_map<std::string, std::string> TX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}};
static const std::unordered_map<std::string, std::string> RX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}, {"RX2", ANTENNA_RX}};
enum class rx_ant_t { LOOPBACK, TRX, RX, TERM };

static constexpr char RFDC_NCO[] = "rfdc";

static const std::vector<std::string> HBX_LOS = {HBX_LO, RFDC_NCO};

static constexpr size_t HBX_MAX_NUM_CHANS = 1;

// These are addresses for the various table-based registers
static constexpr uint8_t ATR_ADDR_0X     = 0;
static constexpr uint8_t ATR_ADDR_RX     = 1;
static constexpr uint8_t ATR_ADDR_TX     = 2;
static constexpr uint8_t ATR_ADDR_XX     = 3; // Full-duplex
static constexpr uint32_t NUM_ATR_STATES = 4;
// Helper for looping
static constexpr std::array<uint8_t, NUM_ATR_STATES> ATR_ADDRS{
    ATR_ADDR_0X, ATR_ADDR_RX, ATR_ADDR_TX, ATR_ADDR_XX};

// In which mode do we operate channels on this device
static const std::vector<ch_mode> HBX_CH_MODES = {ch_mode::REAL, ch_mode::IQ};

// The HBX uses a converter in Real mode for frequencies below 500 MHz and two different
// converters in I and Q mode for frequencies above 500 MHz.
static constexpr double HBX_SWITCH_FREQ = 500e6; // Hz

// The values assigned to the data path properties correspond to what is defined in the
// register map.
static constexpr size_t DATA_PATH_IQ   = 0;
static constexpr size_t DATA_PATH_REAL = 1;

// Register offsets for IQ impairments and DC offset correction
const uint32_t RF_CORE_ADDR_OFFSET      = 0xA000;
const uint32_t IQ_IMPAIRMENTS_TX_OFFSET = 0x0;
const uint32_t IQ_IMPAIRMENTS_RX_OFFSET = 0x20;
const uint32_t NUM_COEFFS_REG_OFFSET    = 0x0;
const uint32_t GROUP_DELAY_REG_OFFSET   = 0x4;
const uint32_t IINLINE_COEFF_REG_OFFSET = 0x8;
const uint32_t ICROSS_COEFF_REG_OFFSET  = 0xC;
const uint32_t QINLINE_COEFF_REG_OFFSET = 0x10;
const uint32_t COEFFS_FRAC_BITS         = 23;
const uint32_t COEFF_WIDTH              = 25;
const uint32_t DC_TX_OFFSET             = 0x40;
const uint32_t DC_RX_OFFSET             = 0x50;
const uint32_t DC_CTRL_REG_OFFSET       = 0x0;
const uint32_t DC_VALUE_OFFSET          = 0x4;

// Default filter setting for IQ impairments filter:
const iq_dc_cal_coeffs_t IQ_DC_DEFAULT_VALUES = iq_dc_cal_coeffs_t{
    1.0, std::vector<std::complex<double>>(1, {0.0, 1.0}), 0.0, {0.0, 0.0}};

// Scaling factor to convert from ADC DC offset to our system DC offset values
const double RFDC_DC_CONV_FACTOR = 2147483648; // 2^31

constexpr char HW_GAIN_STAGE[] = "hw";

static constexpr double RX_MIN_GAIN         = 0;
static constexpr double HBX_DEFAULT_RX_GAIN = RX_MIN_GAIN;

static constexpr double TX_MIN_GAIN         = 0;
static constexpr double HBX_DEFAULT_TX_GAIN = TX_MIN_GAIN;

static constexpr char HBX_GAIN_STAGE_RF[]           = "RF_DSA";
static constexpr char HBX_GAIN_STAGE_LO[]           = "LO_DSA";
static constexpr char HBX_GAIN_STAGE_LO_PWR_INT[]   = "LO_PWR_INT";
static constexpr char HBX_GAIN_STAGE_LO_PWR_EXT[]   = "LO_PWR_EXT";
static constexpr char HBX_GAIN_STAGE_LF_DSA1[]      = "LF_DSA1";
static constexpr char HBX_GAIN_STAGE_LF_DSA2[]      = "LF_DSA2";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA_ALL[] = "ADMV_DSA_ALL";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA1[]    = "ADMV_DSA1";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA2[]    = "ADMV_DSA2";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA3[]    = "ADMV_DSA3";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA4[]    = "ADMV_DSA4";
static constexpr char HBX_GAIN_STAGE_ADMV_DSA5[]    = "ADMV_DSA5";
static constexpr char HBX_GAIN_STAGE_ALL[]          = "all";

static const std::vector<std::string> HBX_RX_GAIN_STAGES_MANUAL = {
    HBX_GAIN_STAGE_RF,
    HBX_GAIN_STAGE_LO,
    HBX_GAIN_STAGE_LO_PWR_INT,
    HBX_GAIN_STAGE_LO_PWR_EXT,
    HBX_GAIN_STAGE_LF_DSA1,
    HBX_GAIN_STAGE_LF_DSA2,
    HBX_GAIN_STAGE_ADMV_DSA_ALL,
    HBX_GAIN_STAGE_ADMV_DSA2,
    HBX_GAIN_STAGE_ADMV_DSA3,
    HBX_GAIN_STAGE_ADMV_DSA4,
    HBX_GAIN_STAGE_ADMV_DSA5,
};

static const std::vector<std::string> HBX_RX_GAIN_STAGES_DEFAULT = {
    HBX_GAIN_STAGE_ALL, HBX_GAIN_STAGE_LO_PWR_EXT};

static const std::vector<std::string> HBX_TX_GAIN_STAGES_MANUAL = {HBX_GAIN_STAGE_RF,
    HBX_GAIN_STAGE_LO,
    HBX_GAIN_STAGE_LO_PWR_INT,
    HBX_GAIN_STAGE_LO_PWR_EXT,
    HBX_GAIN_STAGE_ADMV_DSA_ALL,
    HBX_GAIN_STAGE_ADMV_DSA1,
    HBX_GAIN_STAGE_ADMV_DSA2};

static const std::vector<std::string> HBX_TX_GAIN_STAGES_DEFAULT = {
    HBX_GAIN_STAGE_ALL, HBX_GAIN_STAGE_LO_PWR_EXT};

static constexpr char HBX_GAIN_PROFILE_DEFAULT[]        = "default";
static constexpr char HBX_GAIN_PROFILE_MANUAL[]         = "manual";
static const std::vector<std::string> HBX_GAIN_PROFILES = {
    HBX_GAIN_PROFILE_DEFAULT, HBX_GAIN_PROFILE_MANUAL};

static constexpr uint8_t RF_DSA_MAX_ATTENUATION    = 46; // register value
static constexpr uint8_t LO_LF_DSA_MAX_ATTENUATION = 31; // register value
static constexpr uint8_t LO_DSA_DEFAULT_GAIN       = 29; // dB
static constexpr uint8_t ADMV_DSA_MAX_ATTENUATION  = 15; // dB
// DSA2 in ADMV1420 is special
static constexpr uint8_t ADMV1420_DSA2_MAX_ATTENUATION = 6; // dB
// Maximum value for LO Power: "Higher numbers give more output power." (sic)
static constexpr uint8_t LO_MAX_PWR = 63;

static constexpr double LMX2572_MIN_FREQ     = 500e6; // Hz
static constexpr double LMX2572_MAX_FREQ     = 6e9; // Hz
static constexpr double LMX2572_DEFAULT_FREQ = 1e9; // Hz

static constexpr uint32_t HBX_LO_LOCK_TIMEOUT_MS = 20; // milliseconds

// clang-format off
static const std::vector<hbx_lo_gain_map_item_t> tx_lo_gain_map = {
//  | start_freq | stop_freq | lo_pwr | lo_gain |
    {  0,             500e6,      0,       0    },
    {  500e6,         3e9,       10,      25    },
    {  3e9,           4e9,       30,      24    },
    {  4e9,           4.5e9,     30,      25    },
    {  4.5e9,         6e9,       63,      24    },
    {  6e9,          20e9,       63,      31    }};

static const std::vector<hbx_lo_gain_map_item_t> rx_lo_gain_map = {
//  | start_freq | stop_freq | lo_pwr | lo_gain |
    {  0,             500e6,      0,       0    },
    {  500e6,         1.5e9,     10,      31    },
    {  1.5e9,         4e9,       30,      28    },
    {  4e9,           5e9,       50,      29    },
    {  5e9,          20e9,       63,      31    }};

// These maps take care that we have the optimal LO power at the mixer given an input
// LO power level of about 0 dBm at the LO port.
static const std::vector<std::pair<uhd::range_t, uint8_t>> tx_lo_import_gain_map = {
    // Freq range (Hz)           LO_gain
    {uhd::range_t(0, 500e6),        0},
    {uhd::range_t(500e6, 1e9),     14},
    {uhd::range_t(1e9,   3e9),     13},
    {uhd::range_t(3e9,   3.5e9),   19},
    {uhd::range_t(3.5e9, 4.1e9),   20},
    {uhd::range_t(4.1e9, 4.5e9),   21},
    {uhd::range_t(4.5e9, 5.1e9),   22},
    {uhd::range_t(5.1e9, 5.5e9),   24},
    {uhd::range_t(5.5e9, 6e9),     25},
    {uhd::range_t(6e9,   20e9),    31},
};

static const std::vector<std::pair<uhd::range_t, uint8_t>> rx_lo_import_gain_map = {
    // Freq range (Hz)           LO_gain
    {uhd::range_t(0, 500e6),        0},
    {uhd::range_t(500e6, 1e9),     20},
    {uhd::range_t(1e9,   1.5e9),   19},
    {uhd::range_t(1.5e9, 3e9),     26},
    {uhd::range_t(3e9,   3.5e9),   27},
    {uhd::range_t(3.5e9, 4.2e9),   28},
    {uhd::range_t(4.2e9, 5e9),     29},
    {uhd::range_t(5e9,  20e9),     31},
};

// This map ensures we always export the LOs at about 0 dBm at the output port.
static const std::vector<std::pair<uhd::range_t, uint8_t>> lo_export_power_map = {
    // Freq range (Hz)           LO Power
    {uhd::range_t(0, 500e6),         0},
    {uhd::range_t(500e6, 800e6), 	22},
    {uhd::range_t(800e6, 1e9),  	21},
    {uhd::range_t(1e9, 1.2e9),  	20},
    {uhd::range_t(1.2e9, 1.4e9),  	19},
    {uhd::range_t(1.4e9, 1.6e9),  	19},
    {uhd::range_t(1.6e9, 3e9),  	32},
    {uhd::range_t(3e9, 3.7e9),  	33},
    {uhd::range_t(3.7e9, 4e9),  	35},
    {uhd::range_t(4e9, 4.6e9),  	36},
    {uhd::range_t(4.6e9, 5e9),  	38},
    {uhd::range_t(5e9, 6e9),  	    39},

};

// clang-format on

// Turn clang-formatting off so it doesn't compress these tables into a mess.
// clang-format off

static const std::vector<hbx_tune_map_item_t> tx_tune_map = {
    //  | start_tune_freq | stop_tune_freq | tx_band | rx_band1_subband | tx_filter_branch | lo_filter_branch | rfdc_mode    |    admv1320_band |  lo_divider  |
        {        10e6,           500e6,          0,            0,                0,                  {},       DATA_PATH_REAL,            0,          0.0       },
        {       500e6,           600e6,          1,            0,                1,                 {1},         DATA_PATH_IQ,            0,          1.0       },
        {       600e6,           750e6,          1,            0,                2,                 {1},         DATA_PATH_IQ,            0,          1.0       },
        {       750e6,           950e6,          1,            0,                3,                 {1},         DATA_PATH_IQ,            0,          1.0       },
        {       950e6,          1000e6,          1,            0,                4,                 {1},         DATA_PATH_IQ,            0,          1.0       },
        {      1000e6,          1400e6,          1,            0,                4,                 {2},         DATA_PATH_IQ,            0,          1.0       },
        {      1400e6,          2100e6,          1,            0,                5,                 {2},         DATA_PATH_IQ,            0,          1.0       },
        {      2100e6,          2200e6,          1,            0,                5,                 {3},         DATA_PATH_IQ,            0,          1.0       },
        {      2200e6,          2900e6,          1,            0,                6,                 {3},         DATA_PATH_IQ,            0,          1.0       },
        {      2900e6,          4100e6,          1,            0,                7,                 {3},         DATA_PATH_IQ,            0,          1.0       },
        {      4100e6,          4410e6,          1,            0,                8,                 {3},         DATA_PATH_IQ,            0,          1.0       },
        {      4410e6,          6000e6,          1,            0,                8,                 {4},         DATA_PATH_IQ,            0,          1.0       },
        {      6000e6,          7000e6,          2,            0,                1,               {1,4},         DATA_PATH_IQ,            2,          2.0       },
        {      7000e6,          7500e6,          2,            0,                1,               {1,5},         DATA_PATH_IQ,            2,          2.0       },
        {      7500e6,          8500e6,          2,            0,                1,               {2,5},         DATA_PATH_IQ,            2,          2.0       },
        {      8500e6,          9000e6,          2,            0,                1,               {2,6},         DATA_PATH_IQ,            2,          2.0       },
        {      9000e6,         11500e6,          3,            0,                1,               {1,3},         DATA_PATH_IQ,            3,          4.0       },
        {     11500e6,         13500e6,          3,            0,                1,               {1,4},         DATA_PATH_IQ,            3,          4.0       },
        {     13500e6,         14000e6,          3,            0,                2,               {1,4},         DATA_PATH_IQ,            3,          4.0       },
        {     14000e6,         15000e6,          3,            0,                2,               {1,5},         DATA_PATH_IQ,            3,          4.0       },
        {     15000e6,         17000e6,          3,            0,                2,               {2,5},         DATA_PATH_IQ,            3,          4.0       },
        {     17000e6,         20000e6,          3,            0,                2,               {2,6},         DATA_PATH_IQ,            3,          4.0       },
    };

static const std::vector<hbx_tune_map_item_t> rx_tune_map = {
    //  | start_tune_freq | stop_tune_freq | rx_band  | rx_band1_subband | rx_filter_branch | lo_filter_branch   |     rfdc_mode         | admv1420_band | lo_divider   |
        {        10e6,           500e6,          0,            0,                0,                  {},             DATA_PATH_REAL,             0,           0.0       },
        {       500e6,           600e6,          1,            1,                1,                 {1},               DATA_PATH_IQ,             0,           1.0       },
        {       600e6,           750e6,          1,            1,                2,                 {1},               DATA_PATH_IQ,             0,           1.0       },
        {       750e6,           950e6,          1,            1,                3,                 {1},               DATA_PATH_IQ,             0,           1.0       },
        {       950e6,          1000e6,          1,            1,                4,                 {1},               DATA_PATH_IQ,             0,           1.0       },
        {      1000e6,          1400e6,          1,            1,                4,                 {2},               DATA_PATH_IQ,             0,           1.0       },
        {      1400e6,          2100e6,          1,            1,                5,                 {2},               DATA_PATH_IQ,             0,           1.0       },
        {      2100e6,          2200e6,          1,            1,                5,                 {3},               DATA_PATH_IQ,             0,           1.0       },
        {      2200e6,          2900e6,          1,            1,                6,                 {3},               DATA_PATH_IQ,             0,           1.0       },
        {      2900e6,          4100e6,          1,            2,                7,                 {3},               DATA_PATH_IQ,             1,           1.0       },
        {      4100e6,          4410e6,          1,            2,                8,                 {3},               DATA_PATH_IQ,             2,           1.0       },
        {      4410e6,          6000e6,          1,            2,                8,                 {4},               DATA_PATH_IQ,             2,           1.0       },
        {      6000e6,          7000e6,          1,            3,                9,              {1, 4},               DATA_PATH_IQ,             2,           2.0       },
        {      7000e6,          7500e6,          1,            3,                9,              {1, 5},               DATA_PATH_IQ,             2,           2.0       },
        {      7500e6,          8500e6,          1,            3,                9,              {2, 5},               DATA_PATH_IQ,             2,           2.0       },
        {      8500e6,          9000e6,          1,            3,                9,              {2, 6},               DATA_PATH_IQ,             2,           2.0       },
        {      9000e6,         11500e6,          2,            0,                1,              {1, 3},               DATA_PATH_IQ,             3,           4.0       },
        {     11500e6,         13500e6,          2,            0,                1,              {1, 4},               DATA_PATH_IQ,             3,           4.0       },
        {     13500e6,         14000e6,          2,            0,                2,              {1, 4},               DATA_PATH_IQ,             3,           4.0       },
        {     14000e6,         15000e6,          2,            0,                2,              {1, 5},               DATA_PATH_IQ,             3,           4.0       },
        {     15000e6,         17000e6,          2,            0,                2,              {2, 5},               DATA_PATH_IQ,             3,           4.0       },
        {     17000e6,         20000e6,          2,            0,                2,              {2, 6},               DATA_PATH_IQ,             3,           4.0       },
    };
// Turn clang-format back on
// clang-format on

// The maximum value is taken from the table below. If that should ever change, update it
// here, too. The range is a convenience thing, so we don't need to search through the
// complete table.
static const uhd::gain_range_t HBX_TX_GAIN_RANGE(0.0, 68, 1.0);

// Turn clang-formatting off so it doesn't compress these tables into a mess.
// clang-format off
static const std::vector<hbx_tx_gain_map_item_t> TX_GAIN_MAP = {
  // |   start_freq   |   stop_freq    | gain idx | rf_dsa | admv_dsa |
    {    6e+09,          9e+09,              0,         6,        1    },
    {    6e+09,          9e+09,              1,         6,        2    },
    {    6e+09,          9e+09,              2,         6,        3    },
    {    6e+09,          9e+09,              3,         6,        4    },
    {    6e+09,          9e+09,              4,         6,        5    },
    {    6e+09,          9e+09,              5,         6,        6    },
    {    6e+09,          9e+09,              6,         6,        7    },
    {    6e+09,          9e+09,              7,         6,        8    },
    {    6e+09,          9e+09,              8,         6,        9    },
    {    6e+09,          9e+09,              9,         6,       10    },
    {    6e+09,          9e+09,             10,         6,       11    },
    {    6e+09,          9e+09,             11,         6,       12    },
    {    6e+09,          9e+09,             12,         6,       13    },
    {    6e+09,          9e+09,             13,         6,       14    },
    {    6e+09,          9e+09,             14,         6,       15    },
    {    6e+09,          9e+09,             15,         6,       16    },
    {    6e+09,          9e+09,             16,         6,       17    },
    {    6e+09,          9e+09,             17,         6,       18    },
    {    6e+09,          9e+09,             18,         8,       17    },
    {    6e+09,          9e+09,             19,        10,       16    },
    {    6e+09,          9e+09,             20,        10,       17    },
    {    6e+09,          9e+09,             21,        10,       18    },
    {    6e+09,          9e+09,             22,        12,       17    },
    {    6e+09,          9e+09,             23,        12,       18    },
    {    6e+09,          9e+09,             24,        14,       17    },
    {    6e+09,          9e+09,             25,        14,       18    },
    {    6e+09,          9e+09,             26,        16,       17    },
    {    6e+09,          9e+09,             27,        16,       18    },
    {    6e+09,          9e+09,             28,        18,       17    },
    {    6e+09,          9e+09,             29,        18,       18    },
    {    6e+09,          9e+09,             30,        20,       17    },
    {    6e+09,          9e+09,             31,        20,       18    },
    {    6e+09,          9e+09,             32,        22,       17    },
    {    6e+09,          9e+09,             33,        22,       18    },
    {    6e+09,          9e+09,             34,        24,       17    },
    {    6e+09,          9e+09,             35,        24,       18    },
    {    6e+09,          9e+09,             36,        26,       17    },
    {    6e+09,          9e+09,             37,        26,       18    },
    {    6e+09,          9e+09,             38,        28,       17    },
    {    6e+09,          9e+09,             39,        28,       18    },
    {    6e+09,          9e+09,             40,        30,       17    },
    {    6e+09,          9e+09,             41,        30,       18    },
    {    6e+09,          9e+09,             42,        32,       17    },
    {    6e+09,          9e+09,             43,        32,       18    },
    {    6e+09,          9e+09,             44,        34,       17    },
    {    6e+09,          9e+09,             45,        34,       18    },
    {    6e+09,          9e+09,             46,        36,       17    },
    {    6e+09,          9e+09,             47,        36,       18    },
    {    6e+09,          9e+09,             48,        38,       17    },
    {    6e+09,          9e+09,             49,        38,       18    },
    {    6e+09,          9e+09,             50,        40,       17    },
    {    6e+09,          9e+09,             51,        42,       16    },
    {    6e+09,          9e+09,             52,        42,       17    },
    {    6e+09,          9e+09,             53,        44,       16    },
    {    6e+09,          9e+09,             54,        46,       15    },
    {    6e+09,          9e+09,             55,        46,       16    },
    {    6e+09,          9e+09,             56,        46,       17    },
    {    6e+09,          9e+09,             57,        46,       18    },
    {    6e+09,          9e+09,             58,        46,       19    },
    {    6e+09,          9e+09,             59,        46,       20    },
    {    6e+09,          9e+09,             60,        46,       21    },
    {    6e+09,          9e+09,             61,        46,       22    },
    {    6e+09,          9e+09,             62,        46,       23    },
    {    6e+09,          9e+09,             63,        46,       24    },
    {    6e+09,          9e+09,             64,        46,       25    },
    {    6e+09,          9e+09,             65,        46,       26    },
    {    6e+09,          9e+09,             66,        46,       27    },
    {    6e+09,          9e+09,             67,        46,       28    },
    {    6e+09,          9e+09,             68,        46,       29    },
    {    9e+09,          2e+10,             14,         0,       22    },
    {    9e+09,          2e+10,             15,         0,       23    },
    {    9e+09,          2e+10,             16,         0,       24    },
    {    9e+09,          2e+10,             17,         0,       25    },
    {    9e+09,          2e+10,             18,         0,       26    },
    {    9e+09,          2e+10,             19,        26,        0    },
    {    9e+09,          2e+10,             20,        26,        1    },
    {    9e+09,          2e+10,             21,        26,        2    },
    {    9e+09,          2e+10,             22,        26,        3    },
    {    9e+09,          2e+10,             23,        18,       12    },
    {    9e+09,          2e+10,             24,        18,       13    },
    {    9e+09,          2e+10,             25,        18,       14    },
    {    9e+09,          2e+10,             26,        18,       15    },
    {    9e+09,          2e+10,             27,        24,       10    },
    {    9e+09,          2e+10,             28,        28,        7    },
    {    9e+09,          2e+10,             29,        28,        8    },
    {    9e+09,          2e+10,             30,        28,        9    },
    {    9e+09,          2e+10,             31,        28,       10    },
    {    9e+09,          2e+10,             32,        28,       11    },
    {    9e+09,          2e+10,             33,        28,       12    },
    {    9e+09,          2e+10,             34,        28,       13    },
    {    9e+09,          2e+10,             35,        28,       14    },
    {    9e+09,          2e+10,             36,        28,       15    },
    {    9e+09,          2e+10,             37,        28,       16    },
    {    9e+09,          2e+10,             38,        28,       17    },
    {    9e+09,          2e+10,             39,        46,        0    },
    {    9e+09,          2e+10,             40,        46,        1    },
    {    9e+09,          2e+10,             41,        46,        2    },
    {    9e+09,          2e+10,             42,        46,        3    },
    {    9e+09,          2e+10,             43,        46,        4    },
    {    9e+09,          2e+10,             44,        46,        5    },
    {    9e+09,          2e+10,             45,        46,        6    },
    {    9e+09,          2e+10,             46,        46,        7    },
    {    9e+09,          2e+10,             47,        46,        8    },
    {    9e+09,          2e+10,             48,        46,        9    },
    {    9e+09,          2e+10,             49,        46,       10    },
    {    9e+09,          2e+10,             50,        46,       11    },
    {    9e+09,          2e+10,             51,        46,       12    },
    {    9e+09,          2e+10,             52,        46,       13    },
    {    9e+09,          2e+10,             53,        46,       14    },
    {    9e+09,          2e+10,             54,        46,       15    },
    {    9e+09,          2e+10,             55,        46,       16    },
    {    9e+09,          2e+10,             56,        46,       17    },
    {    9e+09,          2e+10,             57,        46,       18    },
    {    9e+09,          2e+10,             58,        46,       19    },
    {    9e+09,          2e+10,             59,        46,       20    },
    {    9e+09,          2e+10,             60,        46,       21    },
    {    9e+09,          2e+10,             61,        46,       22    },
    {    9e+09,          2e+10,             62,        46,       23    },
    {    9e+09,          2e+10,             63,        46,       24    },
    {    9e+09,          2e+10,             64,        46,       25    },
    {    9e+09,          2e+10,             65,        46,       26    },
    {    9e+09,          2e+10,             66,        46,       27    },
    {    9e+09,          2e+10,             67,        46,       28    },
    {    9e+09,          2e+10,             68,        46,       29    },
};

// Turn clang-format back on
// clang-format on

// The maximum value is taken from the table below. If that should ever change, update it
// here, too. The range is a convenience thing, so we don't need to search through the
// complete table.
static const uhd::gain_range_t HBX_RX_GAIN_RANGE(0.0, 48, 1.0);

// Turn clang-formatting off so it doesn't compress these tables into a mess.
// clang-format off

static const std::vector<hbx_rx_gain_map_item_t> RX_GAIN_MAP{
 // | start_freq  | stop_freq   | gain idx | lf_dsa1 | lf_dsa2 | rf_dsa | admv_dsa |
   {     9000000,    500000000,        0,       3,      10,       0,       0 }, // Starting at 9 MHz instead of 10 MHz
   {     9000000,    500000000,        1,       5,       9,       0,       0 }, // as we exclude the start freq in all
   {     9000000,    500000000,        2,       5,      10,       0,       0 }, // other bands, too, and would therefore
   {     9000000,    500000000,        3,       8,       8,       0,       0 }, // not be able to find a gain for 10 MHz.
   {     9000000,    500000000,        4,       8,       9,       0,       0 },
   {     9000000,    500000000,        5,       9,       9,       0,       0 },
   {     9000000,    500000000,        6,      10,       9,       0,       0 },
   {     9000000,    500000000,        7,      11,       9,       0,       0 },
   {     9000000,    500000000,        8,      11,      10,       0,       0 },
   {     9000000,    500000000,        9,      13,       9,       0,       0 },
   {     9000000,    500000000,       10,      15,       8,       0,       0 },
   {     9000000,    500000000,       11,      16,       8,       0,       0 },
   {     9000000,    500000000,       12,      16,       9,       0,       0 },
   {     9000000,    500000000,       13,      18,       8,       0,       0 },
   {     9000000,    500000000,       14,      19,       8,       0,       0 },
   {     9000000,    500000000,       15,      20,       8,       0,       0 },
   {     9000000,    500000000,       16,      20,       9,       0,       0 },
   {     9000000,    500000000,       17,      21,       9,       0,       0 },
   {     9000000,    500000000,       18,      23,       8,       0,       0 },
   {     9000000,    500000000,       19,      24,       8,       0,       0 },
   {     9000000,    500000000,       20,      24,       9,       0,       0 },
   {     9000000,    500000000,       21,      25,       9,       0,       0 },
   {     9000000,    500000000,       22,      27,       8,       0,       0 },
   {     9000000,    500000000,       23,      27,       9,       0,       0 },
   {     9000000,    500000000,       24,      29,       8,       0,       0 },
   {     9000000,    500000000,       25,      29,       9,       0,       0 },
   {     9000000,    500000000,       26,      31,       8,       0,       0 },
   {     9000000,    500000000,       27,      31,       9,       0,       0 },
   {     9000000,    500000000,       28,      31,      10,       0,       0 },
   {     9000000,    500000000,       29,      31,      11,       0,       0 },
   {     9000000,    500000000,       30,      31,      12,       0,       0 },
   {     9000000,    500000000,       31,      31,      13,       0,       0 },
   {     9000000,    500000000,       32,      31,      14,       0,       0 },
   {     9000000,    500000000,       33,      31,      15,       0,       0 },
   {     9000000,    500000000,       34,      31,      16,       0,       0 },
   {     9000000,    500000000,       35,      30,      18,       0,       0 },
   {     9000000,    500000000,       36,      31,      18,       0,       0 },
   {     9000000,    500000000,       37,      31,      19,       0,       0 },
   {     9000000,    500000000,       38,      31,      20,       0,       0 },
   {     9000000,    500000000,       39,      31,      21,       0,       0 },
   {     9000000,    500000000,       40,      31,      22,       0,       0 },
   {     9000000,    500000000,       41,      31,      23,       0,       0 },
   {     9000000,    500000000,       42,      31,      24,       0,       0 },
   {     9000000,    500000000,       43,      31,      25,       0,       0 },
   {     9000000,    500000000,       44,      31,      26,       0,       0 },
   {     9000000,    500000000,       45,      31,      27,       0,       0 },
   {     9000000,    500000000,       46,      31,      28,       0,       0 },
   {     9000000,    500000000,       47,      31,      29,       0,       0 },
   {     9000000,    500000000,       48,      31,      30,       0,       0 },
   {   500000000,    600000000,        0,       3,      10,       0,       0 },
   {   500000000,    600000000,        1,       5,       9,       0,       0 },
   {   500000000,    600000000,        2,       5,      10,       0,       0 },
   {   500000000,    600000000,        3,       6,      10,       0,       0 },
   {   500000000,    600000000,        4,       8,       9,       0,       0 },
   {   500000000,    600000000,        5,       9,       9,       0,       0 },
   {   500000000,    600000000,        6,       9,      10,       0,       0 },
   {   500000000,    600000000,        7,      11,       9,       0,       0 },
   {   500000000,    600000000,        8,      11,      10,       0,       0 },
   {   500000000,    600000000,        9,      13,       9,       0,       0 },
   {   500000000,    600000000,       10,      14,       9,       0,       0 },
   {   500000000,    600000000,       11,      14,      10,       0,       0 },
   {   500000000,    600000000,       12,      16,       9,       0,       0 },
   {   500000000,    600000000,       13,      16,      10,       0,       0 },
   {   500000000,    600000000,       14,      18,       9,       0,       0 },
   {   500000000,    600000000,       15,      19,       9,       0,       0 },
   {   500000000,    600000000,       16,      19,      10,       0,       0 },
   {   500000000,    600000000,       17,      21,       9,       0,       0 },
   {   500000000,    600000000,       18,      21,      10,       0,       0 },
   {   500000000,    600000000,       19,      24,       8,       0,       0 },
   {   500000000,    600000000,       20,      24,       9,       0,       0 },
   {   500000000,    600000000,       21,      25,       9,       0,       0 },
   {   500000000,    600000000,       22,      26,       9,       0,       0 },
   {   500000000,    600000000,       23,      27,       9,       0,       0 },
   {   500000000,    600000000,       24,      27,      10,       0,       0 },
   {   500000000,    600000000,       25,      29,       9,       0,       0 },
   {   500000000,    600000000,       26,      31,       8,       0,       0 },
   {   500000000,    600000000,       27,      31,       9,       0,       0 },
   {   500000000,    600000000,       28,      31,      10,       0,       0 },
   {   500000000,    600000000,       29,      31,      11,       0,       0 },
   {   500000000,    600000000,       30,      31,      12,       0,       0 },
   {   500000000,    600000000,       31,      31,      13,       0,       0 },
   {   500000000,    600000000,       32,      31,      14,       0,       0 },
   {   500000000,    600000000,       33,      31,      15,       0,       0 },
   {   500000000,    600000000,       34,      31,      16,       0,       0 },
   {   500000000,    600000000,       35,      30,      18,       0,       0 },
   {   500000000,    600000000,       36,      31,      18,       0,       0 },
   {   500000000,    600000000,       37,      31,      19,       0,       0 },
   {   500000000,    600000000,       38,      31,      20,       0,       0 },
   {   500000000,    600000000,       39,      31,      21,       0,       0 },
   {   500000000,    600000000,       40,      31,      22,       0,       0 },
   {   500000000,    600000000,       41,      31,      23,       0,       0 },
   {   500000000,    600000000,       42,      31,      24,       0,       0 },
   {   500000000,    600000000,       43,      31,      25,       0,       0 },
   {   500000000,    600000000,       44,      31,      26,       0,       0 },
   {   500000000,    600000000,       45,      31,      27,       0,       0 },
   {   500000000,    600000000,       46,      31,      28,       0,       0 },
   {   500000000,    600000000,       47,      31,      29,       0,       0 },
   {   500000000,    600000000,       48,      31,      30,       0,       0 },
   {   600000000,    750000000,        0,       1,      12,       0,       0 },
   {   600000000,    750000000,        1,       2,      12,       0,       0 },
   {   600000000,    750000000,        2,       4,      11,       0,       0 },
   {   600000000,    750000000,        3,       6,      10,       0,       0 },
   {   600000000,    750000000,        4,       5,      12,       0,       0 },
   {   600000000,    750000000,        5,       9,       9,       0,       0 },
   {   600000000,    750000000,        6,       9,      10,       0,       0 },
   {   600000000,    750000000,        7,      11,       9,       0,       0 },
   {   600000000,    750000000,        8,      11,      10,       0,       0 },
   {   600000000,    750000000,        9,      13,       9,       0,       0 },
   {   600000000,    750000000,       10,      13,      10,       0,       0 },
   {   600000000,    750000000,       11,      15,       9,       0,       0 },
   {   600000000,    750000000,       12,      15,      10,       0,       0 },
   {   600000000,    750000000,       13,      15,      11,       0,       0 },
   {   600000000,    750000000,       14,      18,       9,       0,       0 },
   {   600000000,    750000000,       15,      19,       9,       0,       0 },
   {   600000000,    750000000,       16,      19,      10,       0,       0 },
   {   600000000,    750000000,       17,      21,       9,       0,       0 },
   {   600000000,    750000000,       18,      22,       9,       0,       0 },
   {   600000000,    750000000,       19,      22,      10,       0,       0 },
   {   600000000,    750000000,       20,      24,       9,       0,       0 },
   {   600000000,    750000000,       21,      25,       9,       0,       0 },
   {   600000000,    750000000,       22,      26,       9,       0,       0 },
   {   600000000,    750000000,       23,      27,       9,       0,       0 },
   {   600000000,    750000000,       24,      27,      10,       0,       0 },
   {   600000000,    750000000,       25,      29,       9,       0,       0 },
   {   600000000,    750000000,       26,      30,       9,       0,       0 },
   {   600000000,    750000000,       27,      31,       9,       0,       0 },
   {   600000000,    750000000,       28,      31,      10,       0,       0 },
   {   600000000,    750000000,       29,      31,      11,       0,       0 },
   {   600000000,    750000000,       30,      31,      12,       0,       0 },
   {   600000000,    750000000,       31,      31,      13,       0,       0 },
   {   600000000,    750000000,       32,      31,      14,       0,       0 },
   {   600000000,    750000000,       33,      31,      15,       0,       0 },
   {   600000000,    750000000,       34,      31,      16,       0,       0 },
   {   600000000,    750000000,       35,      30,      18,       0,       0 },
   {   600000000,    750000000,       36,      31,      18,       0,       0 },
   {   600000000,    750000000,       37,      31,      19,       0,       0 },
   {   600000000,    750000000,       38,      31,      20,       0,       0 },
   {   600000000,    750000000,       39,      31,      21,       0,       0 },
   {   600000000,    750000000,       40,      31,      22,       0,       0 },
   {   600000000,    750000000,       41,      31,      23,       0,       0 },
   {   600000000,    750000000,       42,      31,      24,       0,       0 },
   {   600000000,    750000000,       43,      31,      25,       0,       0 },
   {   600000000,    750000000,       44,      31,      26,       0,       0 },
   {   600000000,    750000000,       45,      31,      27,       0,       0 },
   {   600000000,    750000000,       46,      31,      28,       0,       0 },
   {   600000000,    750000000,       47,      31,      29,       0,       0 },
   {   600000000,    750000000,       48,      31,      30,       0,       0 },
   {   750000000,    950000000,        0,       1,      12,       0,       0 },
   {   750000000,    950000000,        1,       4,      10,       0,       0 },
   {   750000000,    950000000,        2,       5,      10,       0,       0 },
   {   750000000,    950000000,        3,       6,      10,       0,       0 },
   {   750000000,    950000000,        4,       8,       9,       0,       0 },
   {   750000000,    950000000,        5,       9,       9,       0,       0 },
   {   750000000,    950000000,        6,       9,      10,       0,       0 },
   {   750000000,    950000000,        7,      11,       9,       0,       0 },
   {   750000000,    950000000,        8,      11,      10,       0,       0 },
   {   750000000,    950000000,        9,      13,       9,       0,       0 },
   {   750000000,    950000000,       10,      13,      10,       0,       0 },
   {   750000000,    950000000,       11,      15,       9,       0,       0 },
   {   750000000,    950000000,       12,      15,      10,       0,       0 },
   {   750000000,    950000000,       13,      16,      10,       0,       0 },
   {   750000000,    950000000,       14,      18,       9,       0,       0 },
   {   750000000,    950000000,       15,      19,       9,       0,       0 },
   {   750000000,    950000000,       16,      19,      10,       0,       0 },
   {   750000000,    950000000,       17,      21,       9,       0,       0 },
   {   750000000,    950000000,       18,      22,       9,       0,       0 },
   {   750000000,    950000000,       19,      22,      10,       0,       0 },
   {   750000000,    950000000,       20,      24,       9,       0,       0 },
   {   750000000,    950000000,       21,      25,       9,       0,       0 },
   {   750000000,    950000000,       22,      26,       9,       0,       0 },
   {   750000000,    950000000,       23,      27,       9,       0,       0 },
   {   750000000,    950000000,       24,      27,      10,       0,       0 },
   {   750000000,    950000000,       25,      29,       9,       0,       0 },
   {   750000000,    950000000,       26,      31,       8,       0,       0 },
   {   750000000,    950000000,       27,      31,       9,       0,       0 },
   {   750000000,    950000000,       28,      31,      10,       0,       0 },
   {   750000000,    950000000,       29,      31,      11,       0,       0 },
   {   750000000,    950000000,       30,      31,      12,       0,       0 },
   {   750000000,    950000000,       31,      31,      13,       0,       0 },
   {   750000000,    950000000,       32,      31,      14,       0,       0 },
   {   750000000,    950000000,       33,      31,      15,       0,       0 },
   {   750000000,    950000000,       34,      31,      16,       0,       0 },
   {   750000000,    950000000,       35,      30,      18,       0,       0 },
   {   750000000,    950000000,       36,      31,      18,       0,       0 },
   {   750000000,    950000000,       37,      31,      19,       0,       0 },
   {   750000000,    950000000,       38,      31,      20,       0,       0 },
   {   750000000,    950000000,       39,      31,      21,       0,       0 },
   {   750000000,    950000000,       40,      31,      22,       0,       0 },
   {   750000000,    950000000,       41,      31,      23,       0,       0 },
   {   750000000,    950000000,       42,      31,      24,       0,       0 },
   {   750000000,    950000000,       43,      31,      25,       0,       0 },
   {   750000000,    950000000,       44,      31,      26,       0,       0 },
   {   750000000,    950000000,       45,      31,      27,       0,       0 },
   {   750000000,    950000000,       46,      31,      28,       0,       0 },
   {   750000000,    950000000,       47,      31,      29,       0,       0 },
   {   750000000,    950000000,       48,      31,      30,       0,       0 },
   {   950000000,   1400000000,        0,       4,       9,       0,       0 },
   {   950000000,   1400000000,        1,       4,      10,       0,       0 },
   {   950000000,   1400000000,        2,       5,      10,       0,       0 },
   {   950000000,   1400000000,        3,       6,      10,       0,       0 },
   {   950000000,   1400000000,        4,       8,       9,       0,       0 },
   {   950000000,   1400000000,        5,       9,       9,       0,       0 },
   {   950000000,   1400000000,        6,       9,      10,       0,       0 },
   {   950000000,   1400000000,        7,      10,      10,       0,       0 },
   {   950000000,   1400000000,        8,      12,       9,       0,       0 },
   {   950000000,   1400000000,        9,      13,       9,       0,       0 },
   {   950000000,   1400000000,       10,      14,       9,       0,       0 },
   {   950000000,   1400000000,       11,      16,       8,       0,       0 },
   {   950000000,   1400000000,       12,      16,       9,       0,       0 },
   {   950000000,   1400000000,       13,      16,      10,       0,       0 },
   {   950000000,   1400000000,       14,      18,       9,       0,       0 },
   {   950000000,   1400000000,       15,      19,       9,       0,       0 },
   {   950000000,   1400000000,       16,      19,      10,       0,       0 },
   {   950000000,   1400000000,       17,      21,       9,       0,       0 },
   {   950000000,   1400000000,       18,      22,       9,       0,       0 },
   {   950000000,   1400000000,       19,      23,       9,       0,       0 },
   {   950000000,   1400000000,       20,      24,       9,       0,       0 },
   {   950000000,   1400000000,       21,      25,       9,       0,       0 },
   {   950000000,   1400000000,       22,      26,       9,       0,       0 },
   {   950000000,   1400000000,       23,      27,       9,       0,       0 },
   {   950000000,   1400000000,       24,      27,      10,       0,       0 },
   {   950000000,   1400000000,       25,      29,       9,       0,       0 },
   {   950000000,   1400000000,       26,      31,       8,       0,       0 },
   {   950000000,   1400000000,       27,      31,       9,       0,       0 },
   {   950000000,   1400000000,       28,      31,      10,       0,       0 },
   {   950000000,   1400000000,       29,      31,      11,       0,       0 },
   {   950000000,   1400000000,       30,      31,      12,       0,       0 },
   {   950000000,   1400000000,       31,      31,      13,       0,       0 },
   {   950000000,   1400000000,       32,      31,      14,       0,       0 },
   {   950000000,   1400000000,       33,      31,      15,       0,       0 },
   {   950000000,   1400000000,       34,      31,      16,       0,       0 },
   {   950000000,   1400000000,       35,      30,      18,       0,       0 },
   {   950000000,   1400000000,       36,      31,      18,       0,       0 },
   {   950000000,   1400000000,       37,      31,      19,       0,       0 },
   {   950000000,   1400000000,       38,      31,      20,       0,       0 },
   {   950000000,   1400000000,       39,      31,      21,       0,       0 },
   {   950000000,   1400000000,       40,      31,      22,       0,       0 },
   {   950000000,   1400000000,       41,      31,      23,       0,       0 },
   {   950000000,   1400000000,       42,      31,      24,       0,       0 },
   {   950000000,   1400000000,       43,      31,      25,       0,       0 },
   {   950000000,   1400000000,       44,      31,      26,       0,       0 },
   {   950000000,   1400000000,       45,      31,      27,       0,       0 },
   {   950000000,   1400000000,       46,      31,      28,       0,       0 },
   {   950000000,   1400000000,       47,      31,      29,       0,       0 },
   {   950000000,   1400000000,       48,      31,      30,       0,       0 },
   {  1400000000,   2200000000,        0,       3,      11,       0,       0 },
   {  1400000000,   2200000000,        1,       3,      12,       0,       0 },
   {  1400000000,   2200000000,        2,       3,      13,       0,       0 },
   {  1400000000,   2200000000,        3,       8,       9,       0,       0 },
   {  1400000000,   2200000000,        4,       8,      10,       0,       0 },
   {  1400000000,   2200000000,        5,       8,      11,       0,       0 },
   {  1400000000,   2200000000,        6,       9,      11,       0,       0 },
   {  1400000000,   2200000000,        7,      11,      10,       0,       0 },
   {  1400000000,   2200000000,        8,      11,      11,       0,       0 },
   {  1400000000,   2200000000,        9,      14,       9,       0,       0 },
   {  1400000000,   2200000000,       10,      16,       8,       0,       0 },
   {  1400000000,   2200000000,       11,      16,       9,       0,       0 },
   {  1400000000,   2200000000,       12,      16,      10,       0,       0 },
   {  1400000000,   2200000000,       13,      18,       9,       0,       0 },
   {  1400000000,   2200000000,       14,      19,       9,       0,       0 },
   {  1400000000,   2200000000,       15,      19,      10,       0,       0 },
   {  1400000000,   2200000000,       16,      19,      11,       0,       0 },
   {  1400000000,   2200000000,       17,      22,       9,       0,       0 },
   {  1400000000,   2200000000,       18,      24,       8,       0,       0 },
   {  1400000000,   2200000000,       19,      24,       9,       0,       0 },
   {  1400000000,   2200000000,       20,      25,       9,       0,       0 },
   {  1400000000,   2200000000,       21,      26,       9,       0,       0 },
   {  1400000000,   2200000000,       22,      27,       9,       0,       0 },
   {  1400000000,   2200000000,       23,      27,      10,       0,       0 },
   {  1400000000,   2200000000,       24,      27,      11,       0,       0 },
   {  1400000000,   2200000000,       25,      30,       9,       0,       0 },
   {  1400000000,   2200000000,       26,      31,       9,       0,       0 },
   {  1400000000,   2200000000,       27,      31,      10,       0,       0 },
   {  1400000000,   2200000000,       28,      31,      11,       0,       0 },
   {  1400000000,   2200000000,       29,      31,      12,       0,       0 },
   {  1400000000,   2200000000,       30,      31,      13,       0,       0 },
   {  1400000000,   2200000000,       31,      31,      14,       0,       0 },
   {  1400000000,   2200000000,       32,      31,      15,       0,       0 },
   {  1400000000,   2200000000,       33,      31,      16,       0,       0 },
   {  1400000000,   2200000000,       34,      30,      18,       0,       0 },
   {  1400000000,   2200000000,       35,      31,      18,       0,       0 },
   {  1400000000,   2200000000,       36,      31,      19,       0,       0 },
   {  1400000000,   2200000000,       37,      31,      20,       0,       0 },
   {  1400000000,   2200000000,       38,      31,      21,       0,       0 },
   {  1400000000,   2200000000,       39,      31,      22,       0,       0 },
   {  1400000000,   2200000000,       40,      31,      23,       0,       0 },
   {  1400000000,   2200000000,       41,      31,      24,       0,       0 },
   {  1400000000,   2200000000,       42,      31,      25,       0,       0 },
   {  1400000000,   2200000000,       43,      31,      26,       0,       0 },
   {  1400000000,   2200000000,       44,      31,      27,       0,       0 },
   {  1400000000,   2200000000,       45,      31,      28,       0,       0 },
   {  1400000000,   2200000000,       46,      31,      29,       0,       0 },
   {  1400000000,   2200000000,       47,      31,      30,       0,       0 },
   {  2200000000,   2900000000,        0,       3,      11,       0,       0 },
   {  2200000000,   2900000000,        1,       3,      12,       0,       0 },
   {  2200000000,   2900000000,        2,       3,      13,       0,       0 },
   {  2200000000,   2900000000,        3,       8,       9,       0,       0 },
   {  2200000000,   2900000000,        4,       8,      10,       0,       0 },
   {  2200000000,   2900000000,        5,       9,      10,       0,       0 },
   {  2200000000,   2900000000,        6,      10,      10,       0,       0 },
   {  2200000000,   2900000000,        7,      11,      10,       0,       0 },
   {  2200000000,   2900000000,        8,      11,      11,       0,       0 },
   {  2200000000,   2900000000,        9,      15,       8,       0,       0 },
   {  2200000000,   2900000000,       10,      16,       8,       0,       0 },
   {  2200000000,   2900000000,       11,      16,       9,       0,       0 },
   {  2200000000,   2900000000,       12,      16,      10,       0,       0 },
   {  2200000000,   2900000000,       13,      16,      11,       0,       0 },
   {  2200000000,   2900000000,       14,      18,      10,       0,       0 },
   {  2200000000,   2900000000,       15,      19,      10,       0,       0 },
   {  2200000000,   2900000000,       16,      19,      11,       0,       0 },
   {  2200000000,   2900000000,       17,      24,       7,       0,       0 },
   {  2200000000,   2900000000,       18,      24,       8,       0,       0 },
   {  2200000000,   2900000000,       19,      24,       9,       0,       0 },
   {  2200000000,   2900000000,       20,      25,       9,       0,       0 },
   {  2200000000,   2900000000,       21,      26,       9,       0,       0 },
   {  2200000000,   2900000000,       22,      27,       9,       0,       0 },
   {  2200000000,   2900000000,       23,      29,       8,       0,       0 },
   {  2200000000,   2900000000,       24,      30,       8,       0,       0 },
   {  2200000000,   2900000000,       25,      31,       8,       0,       0 },
   {  2200000000,   2900000000,       26,      31,       9,       0,       0 },
   {  2200000000,   2900000000,       27,      31,      10,       0,       0 },
   {  2200000000,   2900000000,       28,      31,      11,       0,       0 },
   {  2200000000,   2900000000,       29,      31,      12,       0,       0 },
   {  2200000000,   2900000000,       30,      31,      13,       0,       0 },
   {  2200000000,   2900000000,       31,      31,      14,       0,       0 },
   {  2200000000,   2900000000,       32,      31,      15,       0,       0 },
   {  2200000000,   2900000000,       33,      31,      16,       0,       0 },
   {  2200000000,   2900000000,       34,      30,      18,       0,       0 },
   {  2200000000,   2900000000,       35,      31,      18,       0,       0 },
   {  2200000000,   2900000000,       36,      31,      19,       0,       0 },
   {  2200000000,   2900000000,       37,      31,      20,       0,       0 },
   {  2200000000,   2900000000,       38,      31,      21,       0,       0 },
   {  2200000000,   2900000000,       39,      31,      22,       0,       0 },
   {  2200000000,   2900000000,       40,      31,      23,       0,       0 },
   {  2200000000,   2900000000,       41,      31,      24,       0,       0 },
   {  2200000000,   2900000000,       42,      31,      25,       0,       0 },
   {  2200000000,   2900000000,       43,      31,      26,       0,       0 },
   {  2200000000,   2900000000,       44,      31,      27,       0,       0 },
   {  2200000000,   2900000000,       45,      31,      28,       0,       0 },
   {  2200000000,   2900000000,       46,      31,      29,       0,       0 },
   {  2200000000,   2900000000,       47,      31,      30,       0,       0 },
   {  2900000000,   4100000000,        0,       0,       0,      12,      31 },
   {  2900000000,   4100000000,        1,       0,       0,      14,      30 },
   {  2900000000,   4100000000,        2,       0,       0,      14,      31 },
   {  2900000000,   4100000000,        3,       0,       0,      14,      32 },
   {  2900000000,   4100000000,        4,       0,       0,      20,      27 },
   {  2900000000,   4100000000,        5,       0,       0,      20,      28 },
   {  2900000000,   4100000000,        6,       0,       0,      20,      29 },
   {  2900000000,   4100000000,        7,       0,       0,      20,      30 },
   {  2900000000,   4100000000,        8,       0,       0,      24,      27 },
   {  2900000000,   4100000000,        9,       0,       0,      24,      28 },
   {  2900000000,   4100000000,       10,       0,       0,      24,      29 },
   {  2900000000,   4100000000,       11,       0,       0,      26,      28 },
   {  2900000000,   4100000000,       12,       0,       0,      28,      27 },
   {  2900000000,   4100000000,       13,       0,       0,      28,      28 },
   {  2900000000,   4100000000,       14,       0,       0,      28,      29 },
   {  2900000000,   4100000000,       15,       0,       0,      30,      28 },
   {  2900000000,   4100000000,       16,       0,       0,      30,      29 },
   {  2900000000,   4100000000,       17,       0,       0,      30,      30 },
   {  2900000000,   4100000000,       18,       0,       0,      30,      31 },
   {  2900000000,   4100000000,       19,       0,       0,      30,      32 },
   {  2900000000,   4100000000,       20,       0,       0,      36,      27 },
   {  2900000000,   4100000000,       21,       0,       0,      36,      28 },
   {  2900000000,   4100000000,       22,       0,       0,      38,      27 },
   {  2900000000,   4100000000,       23,       0,       0,      40,      26 },
   {  2900000000,   4100000000,       24,       0,       0,      40,      27 },
   {  2900000000,   4100000000,       25,       0,       0,      40,      28 },
   {  2900000000,   4100000000,       26,       0,       0,      40,      29 },
   {  2900000000,   4100000000,       27,       0,       0,      42,      28 },
   {  2900000000,   4100000000,       28,       0,       0,      44,      27 },
   {  2900000000,   4100000000,       29,       0,       0,      44,      28 },
   {  2900000000,   4100000000,       30,       0,       0,      44,      29 },
   {  2900000000,   4100000000,       31,       0,       0,      44,      30 },
   {  2900000000,   4100000000,       32,       0,       0,      46,      29 },
   {  2900000000,   4100000000,       33,       0,       0,      46,      30 },
   {  2900000000,   4100000000,       34,       0,       0,      46,      31 },
   {  2900000000,   4100000000,       35,       0,       0,      46,      32 },
   {  2900000000,   4100000000,       36,       0,       0,      46,      33 },
   {  2900000000,   4100000000,       37,       0,       0,      46,      34 },
   {  2900000000,   4100000000,       38,       0,       0,      46,      35 },
   {  2900000000,   4100000000,       39,       0,       0,      46,      36 },
   {  2900000000,   4100000000,       40,       0,       0,      46,      37 },
   {  4100000000,   6000000000,        0,       0,       0,      10,      35 },
   {  4100000000,   6000000000,        1,       0,       0,      16,      30 },
   {  4100000000,   6000000000,        2,       0,       0,      16,      31 },
   {  4100000000,   6000000000,        3,       0,       0,      16,      32 },
   {  4100000000,   6000000000,        4,       0,       0,      16,      33 },
   {  4100000000,   6000000000,        5,       0,       0,      20,      30 },
   {  4100000000,   6000000000,        6,       0,       0,      20,      31 },
   {  4100000000,   6000000000,        7,       0,       0,      20,      32 },
   {  4100000000,   6000000000,        8,       0,       0,      24,      29 },
   {  4100000000,   6000000000,        9,       0,       0,      24,      30 },
   {  4100000000,   6000000000,       10,       0,       0,      24,      31 },
   {  4100000000,   6000000000,       11,       0,       0,      26,      30 },
   {  4100000000,   6000000000,       12,       0,       0,      28,      29 },
   {  4100000000,   6000000000,       13,       0,       0,      28,      30 },
   {  4100000000,   6000000000,       14,       0,       0,      28,      31 },
   {  4100000000,   6000000000,       15,       0,       0,      28,      32 },
   {  4100000000,   6000000000,       16,       0,       0,      30,      31 },
   {  4100000000,   6000000000,       17,       0,       0,      30,      32 },
   {  4100000000,   6000000000,       18,       0,       0,      30,      33 },
   {  4100000000,   6000000000,       19,       0,       0,      30,      34 },
   {  4100000000,   6000000000,       20,       0,       0,      36,      29 },
   {  4100000000,   6000000000,       21,       0,       0,      36,      30 },
   {  4100000000,   6000000000,       22,       0,       0,      36,      31 },
   {  4100000000,   6000000000,       23,       0,       0,      40,      28 },
   {  4100000000,   6000000000,       24,       0,       0,      40,      29 },
   {  4100000000,   6000000000,       25,       0,       0,      40,      30 },
   {  4100000000,   6000000000,       26,       0,       0,      40,      31 },
   {  4100000000,   6000000000,       27,       0,       0,      44,      28 },
   {  4100000000,   6000000000,       28,       0,       0,      44,      29 },
   {  4100000000,   6000000000,       29,       0,       0,      44,      30 },
   {  4100000000,   6000000000,       30,       0,       0,      44,      31 },
   {  4100000000,   6000000000,       31,       0,       0,      46,      30 },
   {  4100000000,   6000000000,       32,       0,       0,      46,      31 },
   {  4100000000,   6000000000,       33,       0,       0,      46,      32 },
   {  4100000000,   6000000000,       34,       0,       0,      46,      33 },
   {  4100000000,   6000000000,       35,       0,       0,      46,      34 },
   {  4100000000,   6000000000,       36,       0,       0,      46,      35 },
   {  4100000000,   6000000000,       37,       0,       0,      46,      36 },
   {  4100000000,   6000000000,       38,       0,       0,      46,      37 },
   {  6000000000,   8000000000,        0,       0,       0,      10,      37 },
   {  6000000000,   8000000000,        1,       0,       0,      14,      34 },
   {  6000000000,   8000000000,        2,       0,       0,      16,      33 },
   {  6000000000,   8000000000,        3,       0,       0,      16,      34 },
   {  6000000000,   8000000000,        4,       0,       0,      16,      35 },
   {  6000000000,   8000000000,        5,       0,       0,      16,      36 },
   {  6000000000,   8000000000,        6,       0,       0,      20,      33 },
   {  6000000000,   8000000000,        7,       0,       0,      20,      34 },
   {  6000000000,   8000000000,        8,       0,       0,      20,      35 },
   {  6000000000,   8000000000,        9,       0,       0,      20,      36 },
   {  6000000000,   8000000000,       10,       0,       0,      24,      33 },
   {  6000000000,   8000000000,       11,       0,       0,      24,      34 },
   {  6000000000,   8000000000,       12,       0,       0,      24,      35 },
   {  6000000000,   8000000000,       13,       0,       0,      28,      32 },
   {  6000000000,   8000000000,       14,       0,       0,      28,      33 },
   {  6000000000,   8000000000,       15,       0,       0,      28,      34 },
   {  6000000000,   8000000000,       16,       0,       0,      28,      35 },
   {  6000000000,   8000000000,       17,       0,       0,      30,      34 },
   {  6000000000,   8000000000,       18,       0,       0,      30,      35 },
   {  6000000000,   8000000000,       19,       0,       0,      30,      36 },
   {  6000000000,   8000000000,       20,       0,       0,      30,      37 },
   {  6000000000,   8000000000,       21,       0,       0,      36,      32 },
   {  6000000000,   8000000000,       22,       0,       0,      36,      33 },
   {  6000000000,   8000000000,       23,       0,       0,      36,      34 },
   {  6000000000,   8000000000,       24,       0,       0,      36,      35 },
   {  6000000000,   8000000000,       25,       0,       0,      40,      32 },
   {  6000000000,   8000000000,       26,       0,       0,      40,      33 },
   {  6000000000,   8000000000,       27,       0,       0,      40,      34 },
   {  6000000000,   8000000000,       28,       0,       0,      40,      35 },
   {  6000000000,   8000000000,       29,       0,       0,      44,      32 },
   {  6000000000,   8000000000,       30,       0,       0,      44,      33 },
   {  6000000000,   8000000000,       31,       0,       0,      44,      34 },
   {  6000000000,   8000000000,       32,       0,       0,      44,      35 },
   {  6000000000,   8000000000,       33,       0,       0,      46,      34 },
   {  6000000000,   8000000000,       34,       0,       0,      46,      35 },
   {  6000000000,   8000000000,       35,       0,       0,      46,      36 },
   {  6000000000,   8000000000,       36,       0,       0,      46,      37 },
   {  8000000000,   9000000000,        0,       0,       0,      14,      36 },
   {  8000000000,   9000000000,        1,       0,       0,      14,      37 },
   {  8000000000,   9000000000,        2,       0,       0,      16,      36 },
   {  8000000000,   9000000000,        3,       0,       0,      16,      37 },
   {  8000000000,   9000000000,        4,       0,       0,      16,      38 },
   {  8000000000,   9000000000,        5,       0,       0,      18,      37 },
   {  8000000000,   9000000000,        6,       0,       0,      18,      38 },
   {  8000000000,   9000000000,        7,       0,       0,      20,      37 },
   {  8000000000,   9000000000,        8,       0,       0,      20,      38 },
   {  8000000000,   9000000000,        9,       0,       0,      22,      37 },
   {  8000000000,   9000000000,       10,       0,       0,      22,      38 },
   {  8000000000,   9000000000,       11,       0,       0,      24,      37 },
   {  8000000000,   9000000000,       12,       0,       0,      24,      38 },
   {  8000000000,   9000000000,       13,       0,       0,      26,      37 },
   {  8000000000,   9000000000,       14,       0,       0,      28,      36 },
   {  8000000000,   9000000000,       15,       0,       0,      28,      37 },
   {  8000000000,   9000000000,       16,       0,       0,      28,      38 },
   {  8000000000,   9000000000,       17,       0,       0,      30,      37 },
   {  8000000000,   9000000000,       18,       0,       0,      32,      36 },
   {  8000000000,   9000000000,       19,       0,       0,      32,      37 },
   {  8000000000,   9000000000,       20,       0,       0,      32,      38 },
   {  8000000000,   9000000000,       21,       0,       0,      34,      37 },
   {  8000000000,   9000000000,       22,       0,       0,      36,      36 },
   {  8000000000,   9000000000,       23,       0,       0,      36,      37 },
   {  8000000000,   9000000000,       24,       0,       0,      36,      38 },
   {  8000000000,   9000000000,       25,       0,       0,      40,      35 },
   {  8000000000,   9000000000,       26,       0,       0,      40,      36 },
   {  8000000000,   9000000000,       27,       0,       0,      40,      37 },
   {  8000000000,   9000000000,       28,       0,       0,      40,      38 },
   {  8000000000,   9000000000,       29,       0,       0,      42,      37 },
   {  8000000000,   9000000000,       30,       0,       0,      44,      36 },
   {  8000000000,   9000000000,       31,       0,       0,      44,      37 },
   {  8000000000,   9000000000,       32,       0,       0,      44,      38 },
   {  8000000000,   9000000000,       33,       0,       0,      46,      37 },
   {  8000000000,   9000000000,       34,       0,       0,      46,      38 },
   {  9000000000,  14000000000,        0,       0,       0,      14,       7 },
   {  9000000000,  14000000000,        1,       0,       0,      14,       8 },
   {  9000000000,  14000000000,        2,       0,       0,      14,       9 },
   {  9000000000,  14000000000,        3,       0,       0,      16,       8 },
   {  9000000000,  14000000000,        4,       0,       0,      18,       7 },
   {  9000000000,  14000000000,        5,       0,       0,      18,       8 },
   {  9000000000,  14000000000,        6,       0,       0,      18,       9 },
   {  9000000000,  14000000000,        7,       0,       0,      20,       8 },
   {  9000000000,  14000000000,        8,       0,       0,      20,       9 },
   {  9000000000,  14000000000,        9,       0,       0,      22,       8 },
   {  9000000000,  14000000000,       10,       0,       0,      22,       9 },
   {  9000000000,  14000000000,       11,       0,       0,      22,      10 },
   {  9000000000,  14000000000,       12,       0,       0,      24,       9 },
   {  9000000000,  14000000000,       13,       0,       0,      24,      10 },
   {  9000000000,  14000000000,       14,       0,       0,      28,       7 },
   {  9000000000,  14000000000,       15,       0,       0,      28,       8 },
   {  9000000000,  14000000000,       16,       0,       0,      28,       9 },
   {  9000000000,  14000000000,       17,       0,       0,      30,       8 },
   {  9000000000,  14000000000,       18,       0,       0,      32,       7 },
   {  9000000000,  14000000000,       19,       0,       0,      32,       8 },
   {  9000000000,  14000000000,       20,       0,       0,      32,       9 },
   {  9000000000,  14000000000,       21,       0,       0,      32,      10 },
   {  9000000000,  14000000000,       22,       0,       0,      36,       7 },
   {  9000000000,  14000000000,       23,       0,       0,      36,       8 },
   {  9000000000,  14000000000,       24,       0,       0,      36,       9 },
   {  9000000000,  14000000000,       25,       0,       0,      36,      10 },
   {  9000000000,  14000000000,       26,       0,       0,      38,       9 },
   {  9000000000,  14000000000,       27,       0,       0,      40,       8 },
   {  9000000000,  14000000000,       28,       0,       0,      40,       9 },
   {  9000000000,  14000000000,       29,       0,       0,      40,      10 },
   {  9000000000,  14000000000,       30,       0,       0,      44,       7 },
   {  9000000000,  14000000000,       31,       0,       0,      44,       8 },
   {  9000000000,  14000000000,       32,       0,       0,      44,       9 },
   {  9000000000,  14000000000,       33,       0,       0,      44,      10 },
   {  9000000000,  14000000000,       34,       0,       0,      46,       9 },
   {  9000000000,  14000000000,       35,       0,       0,      46,      10 },
   { 14000000000,  20000000000,        0,       0,       0,      14,      10 },
   { 14000000000,  20000000000,        1,       0,       0,      16,       9 },
   { 14000000000,  20000000000,        2,       0,       0,      16,      10 },
   { 14000000000,  20000000000,        3,       0,       0,      18,       9 },
   { 14000000000,  20000000000,        4,       0,       0,      18,      10 },
   { 14000000000,  20000000000,        5,       0,       0,      20,       9 },
   { 14000000000,  20000000000,        6,       0,       0,      20,      10 },
   { 14000000000,  20000000000,        7,       0,       0,      22,       9 },
   { 14000000000,  20000000000,        8,       0,       0,      22,      10 },
   { 14000000000,  20000000000,        9,       0,       0,      24,       9 },
   { 14000000000,  20000000000,       10,       0,       0,      24,      10 },
   { 14000000000,  20000000000,       11,       0,       0,      26,       9 },
   { 14000000000,  20000000000,       12,       0,       0,      26,      10 },
   { 14000000000,  20000000000,       13,       0,       0,      28,       9 },
   { 14000000000,  20000000000,       14,       0,       0,      28,      10 },
   { 14000000000,  20000000000,       15,       0,       0,      30,       9 },
   { 14000000000,  20000000000,       16,       0,       0,      30,      10 },
   { 14000000000,  20000000000,       17,       0,       0,      32,       9 },
   { 14000000000,  20000000000,       18,       0,       0,      32,      10 },
   { 14000000000,  20000000000,       19,       0,       0,      34,       9 },
   { 14000000000,  20000000000,       20,       0,       0,      34,      10 },
   { 14000000000,  20000000000,       21,       0,       0,      36,       9 },
   { 14000000000,  20000000000,       22,       0,       0,      36,      10 },
   { 14000000000,  20000000000,       23,       0,       0,      38,       9 },
   { 14000000000,  20000000000,       24,       0,       0,      38,      10 },
   { 14000000000,  20000000000,       25,       0,       0,      40,       9 },
   { 14000000000,  20000000000,       26,       0,       0,      40,      10 },
   { 14000000000,  20000000000,       27,       0,       0,      42,       9 },
   { 14000000000,  20000000000,       28,       0,       0,      42,      10 },
   { 14000000000,  20000000000,       29,       0,       0,      44,       9 },
   { 14000000000,  20000000000,       30,       0,       0,      44,      10 },
   { 14000000000,  20000000000,       31,       0,       0,      46,       9 },
   { 14000000000,  20000000000,       32,       0,       0,      46,      10 },

};
// Turn clang-format back on
// clang-format on
}}} // namespace uhd::usrp::hbx
