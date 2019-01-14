//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_NEON_CONSTANTS_HPP
#define INCLUDED_LIBUHD_NEON_CONSTANTS_HPP

#include <cstddef>
#include <string>
#include <vector>

static constexpr size_t FPGPIO_MASTER_RADIO     = 0;
static constexpr size_t TOTAL_RADIO_PORTS       = 2;
static constexpr double AD9361_RX_MIN_BANDWIDTH = 20.0e6; // HZ
static constexpr double AD9361_RX_MAX_BANDWIDTH = 40.0e6; // HZ

static constexpr double AD9361_TX_MIN_BANDWIDTH = 20.0e6; // HZ
static constexpr double AD9361_TX_MAX_BANDWIDTH = 40.0e6; // HZ

static constexpr double AD9361_TX_MIN_FREQ = 47.0e6; // Hz
static constexpr double AD9361_TX_MAX_FREQ = 6.0e9; // Hz

static constexpr double AD9361_RX_MIN_FREQ = 70.0e6; // Hz
static constexpr double AD9361_RX_MAX_FREQ = 6.0e9; // Hz

static constexpr double NEON_RADIO_RATE = 16e6; // Hz

static constexpr double AD9361_MIN_RX_GAIN  = 0.0; // dB
static constexpr double AD9361_MAX_RX_GAIN  = 76; // dB
static constexpr double AD9361_RX_GAIN_STEP = 1.0; // dB
static constexpr double AD9361_MIN_TX_GAIN  = 0.0; // dB
static constexpr double AD9361_MAX_TX_GAIN  = 89.75; // dB
static constexpr double AD9361_TX_GAIN_STEP = 0.25; // dB


static constexpr bool NEON_DEFAULT_AUTO_DC_OFFSET  = true;
static constexpr bool NEON_DEFAULT_AUTO_IQ_BALANCE = true;
static constexpr bool NEON_DEFAULT_AGC_ENABLE      = false;

static constexpr double NEON_DEFAULT_GAIN       = 0.0;
static constexpr double NEON_DEFAULT_FREQ       = 2.4e9; // Hz
static constexpr double NEON_DEFAULT_BANDWIDTH  = 40e6; // Hz
static constexpr char NEON_DEFAULT_RX_ANTENNA[] = "RX2";
static constexpr char NEON_DEFAULT_TX_ANTENNA[] = "TX/RX";

static const std::vector<std::string> NEON_RX_ANTENNAS = {"RX2", "TX/RX"};

static constexpr size_t NEON_NUM_CHANS = 2;

#endif /* INCLUDED_LIBUHD_NEON_CONSTANTS_HPP */
