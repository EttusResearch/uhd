//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0
//

#include "e100_impl.hpp"
#include "eeprom_utils.hpp"
#include <uhd/usrp/mboard_eeprom.hpp>

namespace {

    const uint8_t E100_EEPROM_ADDR = 0x51;

    struct e100_eeprom_map{
        uint16_t vendor;
        uint16_t device;
        unsigned char revision;
        unsigned char content;
        unsigned char model[8];
        unsigned char env_var[16];
        unsigned char env_setting[64];
        unsigned char serial[10];
        unsigned char name[NAME_MAX_LEN];
    };

    template <typename T> static const byte_vector_t to_bytes(const T &item){
        return byte_vector_t(
            reinterpret_cast<const byte_vector_t::value_type *>(&item),
            reinterpret_cast<const byte_vector_t::value_type *>(&item)+sizeof(item)
        );
    }
}

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

mboard_eeprom_t get_mb_eeprom(uhd::i2c_iface::sptr i2c)
{
    auto &iface = i2c;
    uhd::usrp::mboard_eeprom_t mb_eeprom;

#define sizeof_member(struct_name, member_name) \
    sizeof(reinterpret_cast<struct_name*>(0)->member_name)

    const size_t num_bytes = offsetof(e100_eeprom_map, model);
    byte_vector_t map_bytes = iface->read_eeprom(E100_EEPROM_ADDR, 0, num_bytes);
    e100_eeprom_map map; std::memcpy(&map, &map_bytes[0], map_bytes.size());

    mb_eeprom["vendor"] = std::to_string(uhd::ntohx(map.vendor));
    mb_eeprom["device"] = std::to_string(uhd::ntohx(map.device));
    mb_eeprom["revision"] = std::to_string(unsigned(map.revision));
    mb_eeprom["content"] = std::to_string(unsigned(map.content));

    #define load_e100_string_xx(key) mb_eeprom[#key] = bytes_to_string(iface->read_eeprom( \
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, key), sizeof_member(e100_eeprom_map, key) \
    ));

    load_e100_string_xx(model);
    load_e100_string_xx(env_var);
    load_e100_string_xx(env_setting);
    load_e100_string_xx(serial);
    load_e100_string_xx(name);

    return mb_eeprom;
}


void e100_impl::set_mb_eeprom(const mboard_eeprom_t &mb_eeprom)
{
    auto &iface = _dev_i2c_iface;

    if (mb_eeprom.has_key("vendor")) iface->write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, vendor),
        to_bytes(uhd::htonx(boost::lexical_cast<uint16_t>(mb_eeprom["vendor"])))
    );

    if (mb_eeprom.has_key("device")) iface->write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, device),
        to_bytes(uhd::htonx(boost::lexical_cast<uint16_t>(mb_eeprom["device"])))
    );

    if (mb_eeprom.has_key("revision")) iface->write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, revision),
        byte_vector_t(1, boost::lexical_cast<unsigned>(mb_eeprom["revision"]))
    );

    if (mb_eeprom.has_key("content")) iface->write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, content),
        byte_vector_t(1, boost::lexical_cast<unsigned>(mb_eeprom["content"]))
    );

    #define store_e100_string_xx(key) if (mb_eeprom.has_key(#key)) iface->write_eeprom( \
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, key), \
        string_to_bytes(mb_eeprom[#key], sizeof_member(e100_eeprom_map, key)) \
    );

    store_e100_string_xx(model);
    store_e100_string_xx(env_var);
    store_e100_string_xx(env_setting);
    store_e100_string_xx(serial);
    store_e100_string_xx(name);
}
