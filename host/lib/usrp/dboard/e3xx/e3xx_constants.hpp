//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_E3XX_CONSTANTS_HPP
#define INCLUDED_LIBUHD_E3XX_CONSTANTS_HPP

#include <cstddef>
#include <string>
#include <vector>

static constexpr double AD9361_RX_MIN_BANDWIDTH = 20.0e6; // Hz
static constexpr double AD9361_RX_MAX_BANDWIDTH = 40.0e6; // Hz

static constexpr double AD9361_TX_MIN_BANDWIDTH = 20.0e6; // Hz
static constexpr double AD9361_TX_MAX_BANDWIDTH = 40.0e6; // Hz

static constexpr double AD9361_TX_MIN_FREQ = 47.0e6; // Hz
static constexpr double AD9361_TX_MAX_FREQ = 6.0e9; // Hz

static constexpr double AD9361_RX_MIN_FREQ = 70.0e6; // Hz
static constexpr double AD9361_RX_MAX_FREQ = 6.0e9; // Hz

static constexpr double E3XX_RADIO_RATE = 16e6; // Hz

static constexpr double AD9361_MIN_RX_GAIN  = 0.0; // dB
static constexpr double AD9361_MAX_RX_GAIN  = 76; // dB
static constexpr double AD9361_RX_GAIN_STEP = 1.0; // dB
static constexpr double AD9361_MIN_TX_GAIN  = 0.0; // dB
static constexpr double AD9361_MAX_TX_GAIN  = 89.75; // dB
static constexpr double AD9361_TX_GAIN_STEP = 0.25; // dB

static constexpr bool E3XX_DEFAULT_AUTO_DC_OFFSET  = true;
static constexpr bool E3XX_DEFAULT_AUTO_IQ_BALANCE = true;
static constexpr bool E3XX_DEFAULT_AGC_ENABLE      = false;

static constexpr double E3XX_DEFAULT_GAIN       = 0.0;
static constexpr double E3XX_DEFAULT_FREQ       = 2.4e9; // Hz
static constexpr double E3XX_DEFAULT_BANDWIDTH  = 40e6; // Hz
static constexpr char E3XX_DEFAULT_RX_ANTENNA[] = "RX2";
static constexpr char E3XX_DEFAULT_TX_ANTENNA[] = "TX/RX";

static const std::vector<std::string> E3XX_RX_ANTENNAS = {
    E3XX_DEFAULT_RX_ANTENNA, E3XX_DEFAULT_TX_ANTENNA};

static constexpr char E3XX_GPIO_BANK[] = "INT0";

static constexpr size_t E3XX_NUM_CHANS = 2;

static constexpr char TIMING_MODE_2R2T[] = "2R2T";
static constexpr char TIMING_MODE_1R1T[] = "1R1T";
static constexpr char MIMO[]             = "MIMO"; // 2R2T
static constexpr char SISO_TX1[]         = "SISO_TX1"; // 1R1T
static constexpr char SISO_TX0[]         = "SISO_TX0"; // 1R1T
#endif /* INCLUDED_LIBUHD_E3XX_CONSTANTS_HPP */
