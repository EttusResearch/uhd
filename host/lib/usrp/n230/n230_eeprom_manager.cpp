//
// Copyright 2013-2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_eeprom.h"
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/mac_addr.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include "n230_eeprom_manager.hpp"

namespace uhd { namespace usrp { namespace n230 {

const double n230_eeprom_manager::UDP_TIMEOUT_IN_SEC = 2.0;

n230_eeprom_manager::n230_eeprom_manager(const std::string& addr):
    _seq_num(0)
{
    _udp_xport = transport::udp_simple::make_connected(
        addr, BOOST_STRINGIZE(N230_FW_COMMS_FLASH_PROG_PORT));
    read_mb_eeprom();
}

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
    if (len < max_len - 1) buffer[len] = '\0';
}

const mboard_eeprom_t& n230_eeprom_manager::read_mb_eeprom()
{
    boost::mutex::scoped_lock lock(_mutex);

    //Read EEPROM from device
    _transact(N230_FLASH_COMM_CMD_READ_NV_DATA);
    const n230_eeprom_map_t* map_ptr = reinterpret_cast<const n230_eeprom_map_t*>(_response.data);
    const n230_eeprom_map_t& map = *map_ptr;

    uint16_t ver_major = uhd::htonx<uint16_t>(map.data_version_major);
    uint16_t ver_minor = uhd::htonx<uint16_t>(map.data_version_minor);

    _mb_eeprom["product"] = std::to_string(
        uhd::htonx<uint16_t>(map.hw_product));
    _mb_eeprom["revision"] = std::to_string(
        uhd::htonx<uint16_t>(map.hw_revision));
    //The revision_compat field does not exist in version 1.0
    //EEPROM version 1.0 will only exist on HW revision 1 so it is safe to set
    //revision_compat = revision
    if (ver_major == 1 and ver_minor == 0) {
        _mb_eeprom["revision_compat"] = _mb_eeprom["revision"];
    } else {
        _mb_eeprom["revision_compat"] = std::to_string(
            uhd::htonx<uint16_t>(map.hw_revision_compat));
    }
    _mb_eeprom["serial"] = _bytes_to_string(
        map.serial, N230_EEPROM_SERIAL_LEN);

    //Extract ethernet info
    _mb_eeprom["gateway"] = boost::asio::ip::address_v4(
        uhd::htonx<uint32_t>(map.gateway)).to_string();
    for (size_t i = 0; i < N230_MAX_NUM_ETH_PORTS; i++) {
        const std::string n(1, i+'0');
        _mb_eeprom["ip-addr"+n] = boost::asio::ip::address_v4(
            uhd::htonx<uint32_t>(map.eth_info[i].ip_addr)).to_string();
        _mb_eeprom["subnet"+n] = boost::asio::ip::address_v4(
            uhd::htonx<uint32_t>(map.eth_info[i].subnet)).to_string();
        byte_vector_t mac_addr(map.eth_info[i].mac_addr, map.eth_info[i].mac_addr + 6);
        _mb_eeprom["mac-addr"+n] = mac_addr_t::from_bytes(mac_addr).to_string();
    }

    _mb_eeprom["name"] = _bytes_to_string(
        map.user_name, N230_EEPROM_NAME_LEN);

    return _mb_eeprom;
}

void n230_eeprom_manager::write_mb_eeprom(const mboard_eeprom_t& eeprom)
{
    boost::mutex::scoped_lock lock(_mutex);

    _mb_eeprom = eeprom;

    n230_eeprom_map_t* map_ptr = reinterpret_cast<n230_eeprom_map_t*>(_request.data);
    memset(map_ptr, 0xff, sizeof(n230_eeprom_map_t)); //Initialize to erased state
    //Read EEPROM from device
    _transact(N230_FLASH_COMM_CMD_READ_NV_DATA);
    memcpy(map_ptr, _response.data, sizeof(n230_eeprom_map_t));
    n230_eeprom_map_t& map = *map_ptr;

    // Automatic version upgrade handling
    uint16_t old_ver_major = uhd::htonx<uint16_t>(map.data_version_major);
    uint16_t old_ver_minor = uhd::htonx<uint16_t>(map.data_version_minor);

    //The revision_compat field does not exist for version 1.0 so force write it
    //EEPROM version 1.0 will only exist on HW revision 1 so it is safe to set
    //revision_compat = revision for the upgrade
    bool force_write_version_compat = (old_ver_major == 1 and old_ver_minor == 0);

    map.data_version_major = uhd::htonx<uint16_t>(N230_EEPROM_VER_MAJOR);
    map.data_version_minor = uhd::htonx<uint16_t>(N230_EEPROM_VER_MINOR);

    if (_mb_eeprom.has_key("product")) {
        map.hw_product = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_mb_eeprom["product"]));
    }
    if (_mb_eeprom.has_key("revision")) {
        map.hw_revision = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_mb_eeprom["revision"]));
    }
    if (_mb_eeprom.has_key("revision_compat")) {
        map.hw_revision_compat = uhd::htonx<uint16_t>(
            boost::lexical_cast<uint16_t>(_mb_eeprom["revision_compat"]));
    } else if (force_write_version_compat) {
        map.hw_revision_compat = map.hw_revision;
    }
    if (_mb_eeprom.has_key("serial")) {
        _string_to_bytes(_mb_eeprom["serial"], N230_EEPROM_SERIAL_LEN, map.serial);
    }

    //Push ethernet info
    if (_mb_eeprom.has_key("gateway")){
        map.gateway = uhd::htonx<uint32_t>(
            boost::asio::ip::address_v4::from_string(_mb_eeprom["gateway"]).to_ulong());
    }
    for (size_t i = 0; i < N230_MAX_NUM_ETH_PORTS; i++) {
        const std::string n(1, i+'0');
        if (_mb_eeprom.has_key("ip-addr"+n)){
            map.eth_info[i].ip_addr = uhd::htonx<uint32_t>(
                boost::asio::ip::address_v4::from_string(_mb_eeprom["ip-addr"+n]).to_ulong());
        }
        if (_mb_eeprom.has_key("subnet"+n)){
            map.eth_info[i].subnet = uhd::htonx<uint32_t>(
                boost::asio::ip::address_v4::from_string(_mb_eeprom["subnet"+n]).to_ulong());
        }
        if (_mb_eeprom.has_key("mac-addr"+n)) {
            byte_vector_t mac_addr = mac_addr_t::from_string(_mb_eeprom["mac-addr"+n]).to_bytes();
            std::copy(mac_addr.begin(), mac_addr.end(), map.eth_info[i].mac_addr);
        }
    }
    //store the name
    if (_mb_eeprom.has_key("name")) {
        _string_to_bytes(_mb_eeprom["name"], N230_EEPROM_NAME_LEN, map.user_name);
    }

    //Write EEPROM to device
    _transact(N230_FLASH_COMM_CMD_WRITE_NV_DATA);
}

void n230_eeprom_manager::_transact(const uint32_t command)
{
    //Load request struct
    _request.flags = uhd::htonx<uint32_t>(N230_FLASH_COMM_FLAGS_ACK | command);
    _request.seq = uhd::htonx<uint32_t>(_seq_num++);

    //Send request
    _flush_xport();
    _udp_xport->send(boost::asio::buffer(&_request, sizeof(_request)));

    //Recv reply
    const size_t nbytes = _udp_xport->recv(boost::asio::buffer(&_response, sizeof(_response)), UDP_TIMEOUT_IN_SEC);
    if (nbytes == 0) throw uhd::io_error("n230_eeprom_manager::_transact failure");

    //Sanity checks
    const size_t flags = uhd::ntohx<uint32_t>(_response.flags);
    UHD_ASSERT_THROW(nbytes == sizeof(_response));
    UHD_ASSERT_THROW(_response.seq == _request.seq);
    UHD_ASSERT_THROW(flags & command);
}

void n230_eeprom_manager::_flush_xport()
{
    char buff[sizeof(n230_flash_prog_t)] = {};
    while (_udp_xport->recv(boost::asio::buffer(buff), 0.0)) {
        /*NOP*/
    }
}

}}};    //namespace
