//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/byte_vector.hpp>
#include <uhd/types/mac_addr.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <string>
#include <vector>

static const size_t SERIAL_LEN = 9;
static const size_t NAME_MAX_LEN = 32 - SERIAL_LEN;

//! convert a string to a byte vector to write to eeprom
uhd::byte_vector_t string_to_uint16_bytes(const std::string &num_str);

//! convert a byte vector read from eeprom to a string
std::string uint16_bytes_to_string(const uhd::byte_vector_t &bytes);
