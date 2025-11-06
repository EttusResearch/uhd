//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/ranges.hpp>
#include <unordered_map>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace fbx {

/******************************************************************************
 * Important: When changing values here, check if that also requires updating
 * the manual (host/docs/fbx.dox). If it also requires changing the website or
 * other sales/marketing material, make sure to let the appropriate people know!
 *****************************************************************************/

// The FBX has a non-configurable analog bandwidth of 400 MHz. At lower frequency,
// the usable bandwidth may be smaller though. For those smaller bandwidths, see
// the tune maps.
static constexpr double FBX_DEFAULT_BANDWIDTH = 1.6e9; // Hz

static constexpr double FBX_MIN_FREQ     = 1e6; // Hz
static constexpr double FBX_MAX_FREQ     = 4e9; // Hz
static constexpr double FBX_DEFAULT_FREQ = 1e9; // Hz
static const uhd::freq_range_t FBX_FREQ_RANGE(FBX_MIN_FREQ, FBX_MAX_FREQ);

static constexpr char FBX_GAIN_PROFILE_DEFAULT[]        = "default";
static const std::vector<std::string> FBX_GAIN_PROFILES = {FBX_GAIN_PROFILE_DEFAULT};

/*** Antenna-related constants ***********************************************/
// TX and RX SMA connectors on the front panel
constexpr char ANTENNA_TXRX[] = "TX/RX0";
constexpr char ANTENNA_RX[]   = "RX1";
// Internal "antenna" ports
constexpr char ANTENNA_CAL_LOOPBACK[] = "CAL_LOOPBACK";
constexpr char ANTENNA_SYNC_INT[]     = "SYNC_INT";
constexpr char ANTENNA_SYNC_EXT[]     = "SYNC_EXT";
constexpr char ANTENNA_TERMINATION[]  = "TERM"; // Only RX path
constexpr char ANTENNA_OFF[]          = "OFF"; // Only RX path
// Default antennas (which are selected at init)
constexpr auto DEFAULT_TX_ANTENNA = ANTENNA_TXRX;
constexpr auto DEFAULT_RX_ANTENNA = ANTENNA_RX;
// Helper lists
static const std::vector<std::string> RX_ANTENNAS = {ANTENNA_TXRX,
    ANTENNA_RX,
    ANTENNA_SYNC_INT,
    ANTENNA_SYNC_EXT,
    ANTENNA_CAL_LOOPBACK,
    ANTENNA_TERMINATION};
static const std::vector<std::string> TX_ANTENNAS = {ANTENNA_TXRX, ANTENNA_CAL_LOOPBACK};
// For branding purposes, FBX changed the antenna names around. For
// existing software, we still accept the old antenna names, but map them to the new ones
static const std::unordered_map<std::string, std::string> TX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}};
static const std::unordered_map<std::string, std::string> RX_ANTENNA_NAME_COMPAT_MAP{
    {"TX/RX", ANTENNA_TXRX}, {"RX2", ANTENNA_RX}};

static constexpr char RFDC_NCO[] = "rfdc";

static const std::vector<std::string> FBX_LOS = {RFDC_NCO};

static constexpr size_t FBX_MAX_NUM_CHANS = 4;

// These are addresses for the various table-based registers
static constexpr uint32_t ATR_ADDR_0X    = 0;
static constexpr uint32_t ATR_ADDR_RX    = 1;
static constexpr uint32_t ATR_ADDR_TX    = 2;
static constexpr uint32_t ATR_ADDR_XX    = 3; // Full-duplex
static constexpr uint32_t NUM_ATR_STATES = 4;
// Helper for looping
static constexpr std::array<uint32_t, NUM_ATR_STATES> ATR_ADDRS{0, 1, 2, 3};

}}} // namespace uhd::usrp::fbx
