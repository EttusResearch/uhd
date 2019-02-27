//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_DEFAULTS_HPP
#define INCLUDED_X300_DEFAULTS_HPP

#include "../device3/device3_impl.hpp"
#include <uhd/transport/udp_simple.hpp> //mtu
#include <string>


namespace uhd { namespace usrp { namespace x300 {

static constexpr size_t NIUSRPRIO_DEFAULT_RPC_PORT = 5444;

static constexpr uint32_t RADIO_DEST_PREFIX_TX = 0;
static constexpr size_t XB_DST_E0              = 0;
static constexpr size_t XB_DST_E1              = 1;
static constexpr size_t XB_DST_PCI             = 2;
static constexpr size_t XB_DST_R0              = 3; // Radio 0 -> Slot A
static constexpr size_t XB_DST_R1              = 4; // Radio 1 -> Slot B
static constexpr size_t XB_DST_CE0             = 5;

static constexpr size_t SRC_ADDR0 = 0;
static constexpr size_t SRC_ADDR1 = 1;
static constexpr size_t DST_ADDR  = 2;

static constexpr double DEFAULT_TICK_RATE = 200e6; // Hz
static constexpr double MAX_TICK_RATE     = 200e6; // Hz
static constexpr double MIN_TICK_RATE     = 184.32e6; // Hz
static constexpr double BUS_CLOCK_RATE    = 187.5e6; // Hz

static const std::string FW_FILE_NAME = "usrp_x300_fw.bin";

// Clock & Time-related defaults
static const std::string DEFAULT_CLOCK_SOURCE = "internal";
static const std::string DEFAULT_TIME_SOURCE  = "internal";
static const bool DEFAULT_TIME_OUTPUT         = true;

static const std::vector<std::string> CLOCK_SOURCE_OPTIONS{
    "internal", "external", "gpsdo"};
static const std::vector<std::string> TIME_SOURCE_OPTIONS{
    "internal", "external", "gpsdo"};
static const std::vector<double> EXTERNAL_FREQ_OPTIONS{
    10e6, 11.52e6, 23.04e6, 30.72e6};

static constexpr size_t RX_SW_BUFF_SIZE_ETH =
    0x2000000; // 32MiB    For an ~8k frame size any size >32MiB is just wasted buffer
               // space
static constexpr size_t RX_SW_BUFF_SIZE_ETH_MACOS = 0x100000; // 1Mib

// The FIFO closest to the DMA controller is 1023 elements deep for RX and 1029 elements
// deep for TX where an element is 8 bytes. The buffers (number of frames * frame size)
// must be aligned to the memory page size.  For the control, we are getting lucky because
// 64 frames * 256 bytes each aligns with the typical page size of 4096 bytes.  Since most
// page sizes are 4096 bytes or some multiple of that, keep the number of frames * frame
// size aligned to it.
static constexpr size_t PCIE_RX_DATA_FRAME_SIZE     = 4096; // bytes
static constexpr size_t PCIE_RX_DATA_NUM_FRAMES     = 4096;
static constexpr size_t PCIE_TX_DATA_FRAME_SIZE     = 4096; // bytes
static constexpr size_t PCIE_TX_DATA_NUM_FRAMES     = 4096;
static constexpr size_t PCIE_MSG_FRAME_SIZE         = 256; // bytes
static constexpr size_t PCIE_MSG_NUM_FRAMES         = 64;
static constexpr size_t PCIE_MAX_CHANNELS           = 6;
static constexpr size_t PCIE_MAX_MUXED_CTRL_XPORTS  = 32;
static constexpr size_t PCIE_MAX_MUXED_ASYNC_XPORTS = 4;

static const size_t DATA_FRAME_MAX_SIZE = 8000; // CHDR packet size in bytes
static const size_t XGE_DATA_FRAME_SEND_SIZE =
    4000; // Reduced to make sure flow control packets are not blocked for too long at
          // high rates
static const size_t XGE_DATA_FRAME_RECV_SIZE = 8000;
static const size_t GE_DATA_FRAME_SEND_SIZE  = 1472;
static const size_t GE_DATA_FRAME_RECV_SIZE  = 1472;

static const size_t ETH_MSG_FRAME_SIZE = uhd::transport::udp_simple::mtu; // bytes
// MTU throttling for ethernet/TX (see above):
static constexpr size_t ETH_DATA_FRAME_MAX_TX_SIZE = 8000;

static constexpr double RECV_OFFLOAD_BUFFER_TIMEOUT = 0.1; // seconds
static constexpr double THREAD_BUFFER_TIMEOUT       = 0.1; // Time in seconds

static constexpr size_t ETH_MSG_NUM_FRAMES  = 64;
static constexpr size_t ETH_DATA_NUM_FRAMES = 32;
static constexpr double DEFAULT_SYSREF_RATE = 10e6;

// Limit the number of initialization threads
static const size_t MAX_INIT_THREADS = 10;

static const size_t MAX_RATE_PCIE   = 800000000; // bytes/s
static const size_t MAX_RATE_10GIGE = (size_t)( // bytes/s
    10e9 / 8 * // wire speed multiplied by percentage of packets that is sample data
    (float(DATA_FRAME_MAX_SIZE - uhd::usrp::DEVICE3_TX_MAX_HDR_LEN)
        / float(DATA_FRAME_MAX_SIZE
                + 8 /* UDP header */ + 20 /* Ethernet header length */)));
static const size_t MAX_RATE_1GIGE  = (size_t)( // bytes/s
    10e9 / 8 * // wire speed multiplied by percentage of packets that is sample data
    (float(GE_DATA_FRAME_RECV_SIZE - uhd::usrp::DEVICE3_TX_MAX_HDR_LEN)
        / float(GE_DATA_FRAME_RECV_SIZE
                + 8 /* UDP header */ + 20 /* Ethernet header length */)));

static constexpr double DEFAULT_EXT_ADC_SELF_TEST_DURATION = 30.0;

}}} /* namespace uhd::usrp::x300 */

#endif /* INCLUDED_X300_DEFAULTS_HPP */
