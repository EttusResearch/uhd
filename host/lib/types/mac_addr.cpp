//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/mac_addr.hpp>
#include <uhd/exception.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <stdint.h>
#include <sstream>

using namespace uhd;

mac_addr_t::mac_addr_t(const byte_vector_t &bytes) : _bytes(bytes){
    UHD_ASSERT_THROW(_bytes.size() == 6);
}

mac_addr_t mac_addr_t::from_bytes(const byte_vector_t &bytes){
    return mac_addr_t(bytes);
}

mac_addr_t mac_addr_t::from_string(const std::string &mac_addr_str){

    byte_vector_t bytes;

    try{
        if (mac_addr_str.size() != 17){
            throw uhd::value_error("expected exactly 17 characters");
        }

        //split the mac addr hex string at the colons
        boost::tokenizer<boost::char_separator<char> > hex_num_toks(
            mac_addr_str, boost::char_separator<char>(":"));
        for(const std::string &hex_str:  hex_num_toks){
            int hex_num;
            std::istringstream iss(hex_str);
            iss >> std::hex >> hex_num;
            bytes.push_back(uint8_t(hex_num));
        }

    }
    catch(std::exception const& e){
        throw uhd::value_error(str(
            boost::format("Invalid mac address: %s\n\t%s") % mac_addr_str % e.what()
        ));
    }

    return mac_addr_t::from_bytes(bytes);
}

byte_vector_t mac_addr_t::to_bytes(void) const{
    return _bytes;
}

std::string mac_addr_t::to_string(void) const{
    std::string addr = "";
    for(uint8_t byte:  this->to_bytes()){
        addr += str(boost::format("%s%02x") % ((addr == "")?"":":") % int(byte));
    }
    return addr;
}
