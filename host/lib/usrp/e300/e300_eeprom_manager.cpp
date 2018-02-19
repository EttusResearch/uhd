//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e300_eeprom_manager.hpp"
#include <uhd/types/mac_addr.hpp>
#include <uhd/utils/byteswap.hpp>

namespace uhd { namespace usrp { namespace e300 {

static const std::string _bytes_to_string(const uint8_t* bytes, size_t max_len)
{
    std::string out;
    for (size_t i = 0; i < max_len; i++) {
        if (bytes[i] < 32 or bytes[i] > 127) return out;
        out += bytes[i];
    }
    return out;
}

static void _string_to_bytes(const std::string &string, size_t max_len, uint8_t* buffer)
{
    byte_vector_t bytes;
    const size_t len = std::min(string.size(), max_len);
    for (size_t i = 0; i < len; i++){
        buffer[i] = string[i];
    }
    if (len < max_len)
        buffer[len] = '\0';
}

e300_eeprom_manager::e300_eeprom_manager(i2c::sptr i2c) : _i2c(i2c)
{
    read_mb_eeprom();
    read_db_eeprom();
}

e300_eeprom_manager::~e300_eeprom_manager(void)
{
}

const mboard_eeprom_t& e300_eeprom_manager::read_mb_eeprom(void)
{
    boost::mutex::scoped_lock(_mutex);

    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(mb_eeprom_map_t));
    mb_eeprom_map_t *map_ptr = reinterpret_cast<mb_eeprom_map_t*>(&bytes[0]);
    memset(map_ptr, 0xff, sizeof(mb_eeprom_map_t));

    // get the old contents
    for(size_t i = 0; i < sizeof(mb_eeprom_map_t); i++)
        bytes[i] = _i2c->get_i2c_reg8(MB_ADDR, i);

    mb_eeprom_map_t &map = *map_ptr;

    _mb_eeprom["product"] = std::to_string(
        uhd::ntohx<uint16_t>(map.hw_product));
    _mb_eeprom["revision"] = std::to_string(
        uhd::ntohx<uint16_t>(map.hw_revision));
    _mb_eeprom["serial"] = _bytes_to_string(
        map.serial, MB_SERIAL_LEN);

    byte_vector_t mac_addr(map.mac_addr, map.mac_addr + 6);
    _mb_eeprom["mac-addr"] = mac_addr_t::from_bytes(mac_addr).to_string();

    _mb_eeprom["name"] = _bytes_to_string(
        map.user_name, MB_NAME_LEN);

    return _mb_eeprom;
}

const dboard_eeprom_t& e300_eeprom_manager::read_db_eeprom(void)
{
    boost::mutex::scoped_lock(_mutex);

    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(db_eeprom_map_t));
    db_eeprom_map_t *map_ptr = reinterpret_cast<db_eeprom_map_t*>(&bytes[0]);
    memset(map_ptr, 0xff, sizeof(db_eeprom_map_t));

    // get the old contents
    for(size_t i = 0; i < sizeof(db_eeprom_map_t); i++)
        bytes[i] = _i2c->get_i2c_reg16(DB_ADDR, i);

    db_eeprom_map_t &map = *map_ptr;

    _db_eeprom.id = uhd::usrp::dboard_id_t::from_uint16(
        uhd::ntohx<uint16_t>(map.hw_product));

    _db_eeprom.revision = std::to_string(
        uhd::ntohx<uint16_t>(map.hw_revision));
    _db_eeprom.serial = _bytes_to_string(
        map.serial, DB_SERIAL_LEN);

    return _db_eeprom;
}

void e300_eeprom_manager::write_db_eeprom(const dboard_eeprom_t& eeprom)
{
    boost::mutex::scoped_lock(_mutex);
    _db_eeprom = eeprom;
    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(db_eeprom_map_t));


    db_eeprom_map_t *map_ptr = reinterpret_cast<db_eeprom_map_t*>(&bytes[0]);
    memset(map_ptr, 0xff, sizeof(db_eeprom_map_t));

    // get the old contents
    for(size_t i = 0; i < sizeof(db_eeprom_map_t); i++)
        bytes[i] = _i2c->get_i2c_reg16(DB_ADDR, i);

    db_eeprom_map_t &map = *map_ptr;

    // set the data version, that can be used to distinguish eeprom layouts
    map.data_version_major = E310_DB_MAP_MAJOR;
    map.data_version_minor = E310_DB_MAP_MINOR;

    if (_db_eeprom.id != dboard_id_t::none()) {
        map.hw_product = uhd::htonx<uint16_t>(
            _db_eeprom.id.to_uint16());
    }

    if (not _db_eeprom.revision.empty()) {
        map.hw_revision = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_db_eeprom.revision));
    }

    if (not _db_eeprom.serial.empty()) {
        _string_to_bytes(_db_eeprom.serial, DB_SERIAL_LEN, map.serial);
    }
    for(size_t i = 0; i < sizeof(mb_eeprom_map_t); i++)
        _i2c->set_i2c_reg16(DB_ADDR, i, bytes[i]);
}

void e300_eeprom_manager::write_mb_eeprom(const mboard_eeprom_t& eeprom)
{
    boost::mutex::scoped_lock(_mutex);
    _mb_eeprom = eeprom;
    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(mb_eeprom_map_t));


    mb_eeprom_map_t *map_ptr = reinterpret_cast<mb_eeprom_map_t*>(&bytes[0]);
    memset(map_ptr, 0xff, sizeof(mb_eeprom_map_t));

    // get the old contents
    for(size_t i = 0; i < sizeof(mb_eeprom_map_t); i++)
        bytes[i] = _i2c->get_i2c_reg8(MB_ADDR, i);

    mb_eeprom_map_t &map = *map_ptr;

    // set the data version, that can be used to distinguish eeprom layouts
    map.data_version_major = E310_MB_MAP_MAJOR;
    map.data_version_minor = E310_MB_MAP_MINOR;


    if (_mb_eeprom.has_key("product")) {
        map.hw_product = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_mb_eeprom["product"]));
    }
    if (_mb_eeprom.has_key("revision")) {
        map.hw_revision = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_mb_eeprom["revision"]));
    }
    if (_mb_eeprom.has_key("serial")) {
        _string_to_bytes(_mb_eeprom["serial"], MB_SERIAL_LEN, map.serial);
    }
    if (_mb_eeprom.has_key("mac-addr")) {
        byte_vector_t mac_addr = mac_addr_t::from_string(_mb_eeprom["mac-addr"]).to_bytes();
        std::copy(mac_addr.begin(), mac_addr.end(), map.mac_addr);
    }

    //store the name
    if (_mb_eeprom.has_key("name")) {
        _string_to_bytes(_mb_eeprom["name"], MB_NAME_LEN, map.user_name);
    }

    for(size_t i = 0; i < sizeof(mb_eeprom_map_t); i++)
        _i2c->set_i2c_reg8(MB_ADDR, i, bytes[i]);

}

e300_eeprom_manager::mboard_t e300_eeprom_manager::get_mb_type(void) const
{
    boost::mutex::scoped_lock(_mutex);
    uint16_t pid = boost::lexical_cast<uint16_t>(
        _mb_eeprom["product"]);
    return get_mb_type(pid);
}

e300_eeprom_manager::mboard_t e300_eeprom_manager::get_mb_type(
    uint16_t pid)
{
    switch (pid) {
    case E300_MB_PID:
        return USRP_E300_MB;

    case E310_SG1_MB_PID:
        return USRP_E310_SG1_MB;

    case E310_SG3_MB_PID:
        return USRP_E310_SG3_MB;

    default:
        return UNKNOWN;
    };
}


std::string e300_eeprom_manager::get_mb_type_string(void) const
{
    boost::mutex::scoped_lock(_mutex);
    uint16_t product = boost::lexical_cast<uint16_t>(
        _mb_eeprom["product"]);
    switch (product) {
    case E300_MB_PID:
        return "E3XX";

    case E310_SG1_MB_PID:
        return "E3XX SG1";

    case E310_SG3_MB_PID:
        return "E3XX SG3";

    default:
        return "UNKNOWN";
    };
}

i2c::sptr e300_eeprom_manager::get_i2c_sptr(void)
{
    return _i2c;
}


}}} // namespace
