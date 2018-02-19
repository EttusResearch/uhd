//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_DEFAULTS_H
#define INCLUDED_N230_DEFAULTS_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <uhd/transport/udp_constants.hpp>

namespace uhd {
namespace usrp {
namespace n230 {

static const double DEFAULT_TICK_RATE       = 46.08e6;
static const double MAX_TICK_RATE           = 50e6;
static const double MIN_TICK_RATE           = 1e6;

static const double DEFAULT_TX_SAMP_RATE    = 1.0e6;
static const double DEFAULT_RX_SAMP_RATE    = 1.0e6;
static const double DEFAULT_DDC_FREQ        = 0.0;
static const double DEFAULT_DUC_FREQ        = 0.0;

static const double DEFAULT_FE_GAIN         = 0.0;
static const double DEFAULT_FE_FREQ         = 1.0e9;
static const double DEFAULT_FE_BW           = 56e6;

static const std::string DEFAULT_TIME_SRC   = "none";
static const std::string DEFAULT_CLOCK_SRC  = "internal";

static const size_t DEFAULT_FRAME_SIZE      = 1500 - 20 - 8; //default ipv4 mtu - ipv4 header - udp header
static const size_t MAX_FRAME_SIZE          = 8000;
static const size_t MIN_FRAME_SIZE          = IP_PROTOCOL_MIN_MTU_SIZE;

static const size_t DEFAULT_NUM_FRAMES      = 32;

//A 1MiB SRAM is shared between two radios so we allocate each
//radio 0.5MiB minus 8 packets worth of buffering to ensure
//that the FIFO does not overflow
static const size_t DEFAULT_SEND_BUFF_SIZE  = 500*1024;
#if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
static const size_t DEFAULT_RECV_BUFF_SIZE  = 0x100000; //1Mib
#elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
static const size_t DEFAULT_RECV_BUFF_SIZE  = 0x2000000;//32MiB
#endif

}}}    //namespace

#endif /* INCLUDED_N230_DEFAULTS_H */
