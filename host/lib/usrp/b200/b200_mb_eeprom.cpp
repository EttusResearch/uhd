//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b200_impl.hpp"
#include <uhdlib/utils/eeprom_utils.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>

namespace {
    /* On the B200, this field indicates the slave address. From the FX3, this
     * address is always 0. */
    static const uint8_t B200_EEPROM_SLAVE_ADDR = 0x04;

    //use char array so we dont need to attribute packed
    struct b200_eeprom_map{
        unsigned char _r[220];
        unsigned char revision[2];
        unsigned char product[2];
        unsigned char name[NAME_MAX_LEN];
        unsigned char serial[SERIAL_LEN];
    };
}

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

mboard_eeprom_t b200_impl::get_mb_eeprom(uhd::i2c_iface::sptr iface)
{
    mboard_eeprom_t mb_eeprom;

    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface->read_eeprom(B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, revision), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface->read_eeprom(B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, product), 2)
    );

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface->read_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface->read_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, name), NAME_MAX_LEN
    ));

    return mb_eeprom;
}

void b200_impl::set_mb_eeprom(const mboard_eeprom_t &mb_eeprom)
{
    //parse the revision number
    if (mb_eeprom.has_key("revision")) _iface->write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) _iface->write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the serial
    if (mb_eeprom.has_key("serial")) _iface->write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) _iface->write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

