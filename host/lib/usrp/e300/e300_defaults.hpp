//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_DEFAULTS_HPP
#define INCLUDED_E300_DEFAULTS_HPP

#include "ad9361_client.h"

namespace uhd { namespace usrp { namespace e300 {

static const double DEFAULT_TICK_RATE       = 32e6;
static const double MIN_TICK_RATE           = 10e6;

static const double DEFAULT_TX_SAMP_RATE    = 1.0e6;
static const double DEFAULT_RX_SAMP_RATE    = 1.0e6;
static const double DEFAULT_DDC_FREQ        = 0.0;
static const double DEFAULT_DUC_FREQ        = 0.0;

static const std::string DEFAULT_TIME_SRC   = "internal";
static const std::string DEFAULT_CLOCK_SRC  = "internal";

static const size_t DEFAULT_RX_DATA_FRAME_SIZE = 4096;
static const size_t DEFAULT_RX_DATA_NUM_FRAMES = 32;

static const size_t DEFAULT_TX_DATA_FRAME_SIZE = 4096;
static const size_t DEFAULT_TX_DATA_NUM_FRAMES = 32;

static const size_t DEFAULT_CTRL_FRAME_SIZE    = 64;
static const size_t DEFAULT_CTRL_NUM_FRAMES    = 32;

static const size_t MAX_NET_RX_DATA_FRAME_SIZE = 1200;
static const size_t MAX_NET_TX_DATA_FRAME_SIZE = 1200;

static const size_t MAX_AXI_RX_DATA_FRAME_SIZE = 4096;
static const size_t MAX_AXI_TX_DATA_FRAME_SIZE = 4096;

static const size_t MAX_DMA_CHANNEL_PAIRS = 16;

static const double AD9361_SPI_RATE = 8e6;

class e300_ad9361_client_t : public ad9361_params {
public:
    ~e300_ad9361_client_t() {}
    double get_band_edge(frequency_band_t band) {
        switch (band) {
        case AD9361_RX_BAND0:   return 1.2e9;
        case AD9361_RX_BAND1:   return 2.6e9;
        case AD9361_TX_BAND0:   return 2940.0e6;
        default:                return 0;
        }
    }
    clocking_mode_t get_clocking_mode() {
        return clocking_mode_t::AD9361_XTAL_N_CLK_PATH;
    }
    digital_interface_mode_t get_digital_interface_mode() {
        return AD9361_DDR_FDD_LVCMOS;
    }
    digital_interface_delays_t get_digital_interface_timing() {
        digital_interface_delays_t delays;
        delays.rx_clk_delay = 0;
        delays.rx_data_delay = 0xF;
        delays.tx_clk_delay = 0;
        delays.tx_data_delay = 0xF;
        return delays;
    }
};

}}} // namespace

#endif // INCLUDED_E300_DEFAULTS_HPP
