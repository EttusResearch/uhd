//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/defaults.hpp>
#include <stdint.h>
#include <string>

namespace uhd { namespace rfnoc {

constexpr uint16_t RFNOC_PROTO_VER = 0x0100;

static const size_t CMD_FIFO_SIZE    = 256; // Lines == multiples of 8 bytes
static const size_t MAX_CMD_PKT_SIZE = 3; // Lines == multiples of 8 bytes

// Regular expressions
static const std::string VALID_BLOCKNAME_REGEX = "[A-Za-z][A-Za-z0-9_]*";
static const std::string DEVICE_NUMBER_REGEX   = R"-((?:(\d+)/)?)-";
static const std::string BLOCK_COUNTER_REGEX   = R"-((?:#(\d+))?)-";
static const std::string VALID_BLOCKID_REGEX =
    DEVICE_NUMBER_REGEX + "(" + VALID_BLOCKNAME_REGEX + ")" + BLOCK_COUNTER_REGEX;
static const std::string MATCH_BLOCKID_REGEX =
    DEVICE_NUMBER_REGEX + "(" + VALID_BLOCKNAME_REGEX + ")?" + BLOCK_COUNTER_REGEX;

}} /* namespace uhd::rfnoc */
