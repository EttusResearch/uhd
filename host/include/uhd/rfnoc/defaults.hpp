//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_DEFAULTS_HPP
#define INCLUDED_LIBUHD_RFNOC_DEFAULTS_HPP

#include <string>

namespace uhd { namespace rfnoc {

static const std::string CLOCK_KEY_GRAPH("__graph__");

static const std::string PROP_KEY_DECIM("decim");
static const std::string PROP_KEY_SAMP_RATE("samp_rate");
static const std::string PROP_KEY_SCALING("scaling");
static const std::string PROP_KEY_TYPE("type");
static const std::string PROP_KEY_FREQ("freq");
static const std::string PROP_KEY_TICK_RATE("tick_rate");
static const std::string PROP_KEY_SPP("spp");

static const std::string NODE_ID_SEP("SEP");

using io_type_t                     = std::string;
static const io_type_t IO_TYPE_SC16 = "sc16";

static const std::string ACTION_KEY_STREAM_CMD("stream_cmd");
static const std::string ACTION_KEY_RX_EVENT("rx_event");

//! If the block name can't be automatically detected, this name is used
static const std::string DEFAULT_BLOCK_NAME = "Block";
//! This NOC-ID is used to look up the default block
static const uint32_t DEFAULT_NOC_ID = 0xFFFFFFFF;
static const double DEFAULT_TICK_RATE = 1.0;
// Whenever we need a default spp value use this, unless there are some
// block/device-specific constraints. It will keep the frame size below 1500.
static const int DEFAULT_SPP = 1996;

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_RFNOC_DEFAULTS_HPP */

