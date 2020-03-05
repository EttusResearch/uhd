//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <boost/assign/list_of.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace basic_and_lf {
constexpr uint32_t BASIC_TX_PID = 0x0000;
constexpr uint32_t BASIC_RX_PID = 0x0001;
constexpr uint32_t LF_TX_PID    = 0x000E;
constexpr uint32_t LF_RX_PID    = 0x000F;
// The PIDs of these dboards are duplicated to support a new operating mode
// on RFNoC devices, while maintaining legacy for the USRP2
constexpr uint32_t RFNOC_PID_FLAG     = 0x6300;
constexpr uint32_t BASIC_TX_RFNOC_PID = BASIC_TX_PID | RFNOC_PID_FLAG; // 0x6300
constexpr uint32_t BASIC_RX_RFNOC_PID = BASIC_RX_PID | RFNOC_PID_FLAG; // 0x6301
constexpr uint32_t LF_TX_RFNOC_PID    = LF_TX_PID | RFNOC_PID_FLAG; // 0x630E
constexpr uint32_t LF_RX_RFNOC_PID    = LF_RX_PID | RFNOC_PID_FLAG; // 0x630F

constexpr double BASIC_MAX_BANDWIDTH = 250e6; // Hz
constexpr double LF_MAX_BANDWIDTH    = 32e6; // Hz


static const std::vector<std::string> rx_frontends{"0", "1"};
static const std::vector<std::string> tx_frontends{"0"};

static const std::map<std::string, double> antenna_mode_bandwidth_scalar{
    {"A", 1.0}, {"B", 1.0}, {"AB", 2.0}, {"BA", 2.0}};

static const uhd::dict<std::string, std::string> antenna_mode_to_conn =
    boost::assign::map_list_of("AB", "IQ")("BA", "QI")("A", "I")("B", "Q");
}}}}; // namespace uhd::usrp::dboard::basic_and_lf
