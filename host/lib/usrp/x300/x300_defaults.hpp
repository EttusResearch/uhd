//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_DEFAULTS_HPP
#define INCLUDED_X300_DEFAULTS_HPP

#include <uhd/transport/udp_simple.hpp> //mtu
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace x300 {

static constexpr size_t NIUSRPRIO_DEFAULT_RPC_PORT = 5444;

static constexpr size_t XB_DST_E0  = 0;
static constexpr size_t XB_DST_E1  = 1;
static constexpr size_t XB_DST_PCI = 2;
static constexpr size_t XB_DST_R0  = 3; // Radio 0 -> Slot A
static constexpr size_t XB_DST_R1  = 4; // Radio 1 -> Slot B
static constexpr size_t XB_DST_CE0 = 5;

static constexpr size_t SRC_ADDR0 = 0;
static constexpr size_t SRC_ADDR1 = 1;
static constexpr size_t DST_ADDR  = 2;

static constexpr double DEFAULT_TICK_RATE   = 200e6; // Hz
static constexpr double MAX_TICK_RATE       = 200e6; // Hz
static constexpr double MIN_TICK_RATE       = 184.32e6; // Hz
static constexpr double BUS_CLOCK_RATE      = 187.5e6; // Hz
static constexpr double DEFAULT_SYSREF_RATE = 10e6;

static const std::string FW_FILE_NAME = "usrp_x300_fw.bin";

// Clock & Time-related defaults
static const std::string DEFAULT_CLOCK_SOURCE = "internal";
static const std::string DEFAULT_TIME_SOURCE  = "internal";
static const bool DEFAULT_TIME_OUTPUT         = true;

static const std::vector<std::string> CLOCK_SOURCE_OPTIONS{
    "internal", "external", "gpsdo"};
static const std::vector<std::string> TIME_SOURCE_OPTIONS{
    "internal", "external", "gpsdo"};
static const std::vector<double> EXTERNAL_FREQ_OPTIONS{10e6, 11.52e6, 23.04e6, 30.72e6};

// Limit the number of initialization threads
static const size_t MAX_INIT_THREADS = 10;

static const size_t DATA_FRAME_MAX_SIZE = 8000; // CHDR packet size in bytes

static constexpr double RECV_OFFLOAD_BUFFER_TIMEOUT = 0.1; // seconds
static constexpr double THREAD_BUFFER_TIMEOUT       = 0.1; // Time in seconds

static constexpr double DEFAULT_EXT_ADC_SELF_TEST_DURATION = 30.0;

}}} /* namespace uhd::usrp::x300 */

#endif /* INCLUDED_X300_DEFAULTS_HPP */
