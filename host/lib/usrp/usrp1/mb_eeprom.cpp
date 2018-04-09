//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp1_impl.hpp"
#include <uhdlib/utils/eeprom_utils.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/byte_vector.hpp>

namespace {
    const uint8_t USRP1_EEPROM_ADDR = 0x50;
    const size_t USRP1_SERIAL_LEN = 8;

    //use char array so we dont need to attribute packed
    struct usrp1_eeprom_map{
        unsigned char _r[221];
        unsigned char mcr[4];
        unsigned char name[NAME_MAX_LEN];
        unsigned char serial[USRP1_SERIAL_LEN];
    };
}

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

mboard_eeprom_t usrp1_impl::get_mb_eeprom(uhd::i2c_iface::sptr iface)
{
    mboard_eeprom_t mb_eeprom;

    //extract the serial
    mb_eeprom["serial"] = uhd::bytes_to_string(iface->read_eeprom(
        USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, serial), USRP1_SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = uhd::bytes_to_string(iface->read_eeprom(
        USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, name), NAME_MAX_LEN
    ));

    //extract master clock rate as a 32-bit uint in Hz
    uint32_t master_clock_rate;
    const byte_vector_t rate_bytes = iface->read_eeprom(
        USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, mcr), sizeof(master_clock_rate)
    );
    std::copy(
        rate_bytes.begin(), rate_bytes.end(), //input
        reinterpret_cast<uint8_t *>(&master_clock_rate) //output
    );
    master_clock_rate = ntohl(master_clock_rate);
    if (master_clock_rate > 1e6 and master_clock_rate < 1e9){
        mb_eeprom["mcr"] = std::to_string(master_clock_rate);
    }
    else mb_eeprom["mcr"] = "";

    return mb_eeprom;
}

void usrp1_impl::set_mb_eeprom(const mboard_eeprom_t &mb_eeprom)
{
    auto &iface = _fx2_ctrl;

    //store the serial
    if (mb_eeprom.has_key("serial")) iface->write_eeprom(
        USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], USRP1_SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface->write_eeprom(
        USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );

    //store the master clock rate as a 32-bit uint in Hz
    if (mb_eeprom.has_key("mcr")){
        uint32_t master_clock_rate = uint32_t(std::stod(mb_eeprom["mcr"]));
        master_clock_rate = htonl(master_clock_rate);
        const byte_vector_t rate_bytes(
            reinterpret_cast<const uint8_t *>(&master_clock_rate),
            reinterpret_cast<const uint8_t *>(&master_clock_rate)
                + sizeof(master_clock_rate)
        );
        iface->write_eeprom(
            USRP1_EEPROM_ADDR, offsetof(usrp1_eeprom_map, mcr), rate_bytes
        );
    }
}

