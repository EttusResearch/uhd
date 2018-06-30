//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/types/byte_vector.hpp>

namespace uhd{

std::string bytes_to_string(const byte_vector_t &bytes){
    std::string out;
    for(uint8_t byte:  bytes){
        if (byte < 32 or byte > 127) return out;
        out += byte;
    }
    return out;
}

byte_vector_t string_to_bytes(const std::string &str, size_t max_length){
    byte_vector_t bytes;
    for (size_t i = 0; i < std::min(str.size(), max_length); i++){
        bytes.push_back(str[i]);
    }
    if (bytes.size() < max_length - 1) bytes.push_back('\0');
    return bytes;
}

} /* namespace uhd */
