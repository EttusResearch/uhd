//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_mb_eeprom.hpp"
#include "b300_regs.hpp"
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhdlib/utils/eeprom_utils.hpp>

namespace {
const uint8_t B300_EEPROM_ADDR = 0x50;

struct b300_eeprom_map
{
    // identifying numbers
    unsigned char revision[2];
    unsigned char product[2];
    unsigned char revision_compat[2];
    uint8_t _pad0[2];

    // names and serials
    unsigned char name[NAME_MAX_LEN];
    unsigned char serial[SERIAL_LEN];
};
} // namespace

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

std::string uhd::usrp::b300::map_pid_to_product_name(const uint32_t pid)
{
    switch (pid) {
        case B310_PID:
        case B310_PCIE_PID:
        case B310_TB_PID:
            return "B310";
        default:
            return "UNKNOWN";
    }
}

mboard_eeprom_t uhd::usrp::b300::get_mb_eeprom(uhd::i2c_iface::sptr iface)
{
    byte_vector_t bytes =
        iface->read_eeprom(B300_EEPROM_ADDR, 0, sizeof(struct b300_eeprom_map));

    mboard_eeprom_t mb_eeprom;
    if (bytes.empty()) {
        UHD_LOG_WARNING("B300 MB EEPROM", "No data read from B300 motherboard EEPROM.");
        return mb_eeprom;
    }

    // extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        byte_vector_t(bytes.begin() + offsetof(b300_eeprom_map, revision),
            bytes.begin() + (offsetof(b300_eeprom_map, revision) + 2)));

    // extract the revision compat number
    mb_eeprom["revision_compat"] = uint16_bytes_to_string(
        byte_vector_t(bytes.begin() + offsetof(b300_eeprom_map, revision_compat),
            bytes.begin() + (offsetof(b300_eeprom_map, revision_compat) + 2)));

    // extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        byte_vector_t(bytes.begin() + offsetof(b300_eeprom_map, product),
            bytes.begin() + (offsetof(b300_eeprom_map, product) + 2)));

    // extract the serial number
    mb_eeprom["serial"] =
        bytes_to_string(byte_vector_t(bytes.begin() + offsetof(b300_eeprom_map, serial),
            bytes.begin() + (offsetof(b300_eeprom_map, serial) + SERIAL_LEN)));

    // extract the name
    mb_eeprom["name"] =
        bytes_to_string(byte_vector_t(bytes.begin() + offsetof(b300_eeprom_map, name),
            bytes.begin() + (offsetof(b300_eeprom_map, name) + NAME_MAX_LEN)));

    return mb_eeprom;
}

void uhd::usrp::b300::set_mb_eeprom(
    i2c_iface::sptr iface, const mboard_eeprom_t& mb_eeprom)
{
    // parse the revision number
    if (mb_eeprom.has_key("revision"))
        iface->write_eeprom(B300_EEPROM_ADDR,
            offsetof(b300_eeprom_map, revision),
            string_to_uint16_bytes(mb_eeprom["revision"]));

    // parse the revision compat number
    if (mb_eeprom.has_key("revision_compat"))
        iface->write_eeprom(B300_EEPROM_ADDR,
            offsetof(b300_eeprom_map, revision_compat),
            string_to_uint16_bytes(mb_eeprom["revision_compat"]));

    // parse the product code
    if (mb_eeprom.has_key("product"))
        iface->write_eeprom(B300_EEPROM_ADDR,
            offsetof(b300_eeprom_map, product),
            string_to_uint16_bytes(mb_eeprom["product"]));

    // parse the serial number
    if (mb_eeprom.has_key("serial"))
        iface->write_eeprom(B300_EEPROM_ADDR,
            offsetof(b300_eeprom_map, serial),
            string_to_bytes(mb_eeprom["serial"], SERIAL_LEN));

    // parse the name
    if (mb_eeprom.has_key("name"))
        iface->write_eeprom(B300_EEPROM_ADDR,
            offsetof(b300_eeprom_map, name),
            string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN));
}
