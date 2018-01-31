//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/eeprom_utils.hpp>
#include <boost/lexical_cast.hpp>

uhd::byte_vector_t string_to_uint16_bytes(const std::string &num_str){
    const uint16_t num = boost::lexical_cast<uint16_t>(num_str);
    const std::vector<uint8_t> lsb_msb = {
        uint8_t(num >> 0),
        uint8_t(num >> 8)
    };
    return lsb_msb;
}

std::string uint16_bytes_to_string(const uhd::byte_vector_t &bytes){
    const uint16_t num = (uint16_t(bytes.at(0)) << 0) | (uint16_t(bytes.at(1)) << 8);
    return (num == 0 or num == 0xffff)? "" : std::to_string(num);
}
