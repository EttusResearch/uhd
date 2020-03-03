//
// Copyright 2017-2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b200_impl.hpp"
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhdlib/utils/eeprom_utils.hpp>
#include <unordered_map>

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

namespace {

constexpr auto LOG_ID = "B2xx_EEPROM";

struct eeprom_field_t
{
    size_t offset;
    size_t length;
};

// EEPROM map information is duplicated in common_const.h for the
// firmware and bootloader code.
// EEPROM map information is duplicated in b2xx_fx3_utils.cpp

constexpr uint16_t SIGNATURE_ADDR = 0x0000;
constexpr size_t SIGNATURE_LENGTH = 4;

constexpr auto EXPECTED_MAGIC  = "45568"; // 0xB200
constexpr auto EXPECTED_COMPAT = "1";

constexpr uint32_t REV1_SIGNATURE = 0xB01A5943;
constexpr uint16_t REV1_BASE_ADDR = 0x7F00;
constexpr size_t REV1_LENGTH      = 46;

const std::unordered_map<std::string, eeprom_field_t> B200_REV1_MAP = {
    {"magic", {0, 2}},
    {"eeprom_revision", {2, 2}},
    {"eeprom_compat", {4, 2}},
    {"vendor_id", {6, 2}},
    {"product_id", {8, 2}},
    {"revision", {10, 2}},
    {"product", {12, 2}},
    {"name", {14, NAME_MAX_LEN}},
    {"serial", {14 + NAME_MAX_LEN, SERIAL_LEN}},
    // pad of 210 bytes
};

constexpr uint32_t REV0_SIGNATURE = 0xB2145943;
constexpr uint16_t REV0_BASE_ADDR = 0x04DC;
constexpr size_t REV0_LENGTH      = 36;

const std::unordered_map<std::string, eeprom_field_t> B200_REV0_MAP = {
    // front pad of 220 bytes
    {"revision", {0, 2}},
    {"product", {2, 2}},
    {"name", {4, NAME_MAX_LEN}},
    {"serial", {4 + NAME_MAX_LEN, SERIAL_LEN}},
};

constexpr int UNKNOWN_REV = -1;

int _get_rev(uhd::i2c_iface::sptr iface)
{
    auto bytes =
        iface->read_eeprom(SIGNATURE_ADDR >> 8, SIGNATURE_ADDR & 0xFF, SIGNATURE_LENGTH);
    uint32_t signature = bytes[3] << 24 | bytes[2] << 16 | bytes[1] << 8 | bytes[0];
    if (signature == REV0_SIGNATURE) {
        return 0;
    } else if (signature == REV1_SIGNATURE) {
        return 1;
    } else {
        UHD_LOG_WARNING(LOG_ID, "Unknown signature! 0x" << std::hex << signature);
        return UNKNOWN_REV;
    }
}

byte_vector_t _get_eeprom(uhd::i2c_iface::sptr iface)
{
    const auto rev = _get_rev(iface);
    if (rev == UNKNOWN_REV) {
        return byte_vector_t();
    }

    const uint16_t addr = (rev == 0) ? REV0_BASE_ADDR : REV1_BASE_ADDR;
    const size_t length = (rev == 0) ? REV0_LENGTH : REV1_LENGTH;

    return iface->read_eeprom(addr >> 8, addr & 0xFF, length);
}

} // namespace

mboard_eeprom_t b200_impl::get_mb_eeprom(uhd::i2c_iface::sptr iface)
{
    auto rev   = _get_rev(iface);
    auto bytes = _get_eeprom(iface);
    mboard_eeprom_t mb_eeprom;
    if (rev == UNKNOWN_REV or bytes.empty()) {
        return mb_eeprom;
    }

    auto eeprom_map = (rev == 0) ? B200_REV0_MAP : B200_REV1_MAP;

    for (const auto& element : eeprom_map) {
        // There is an assumption here that fields of length 2 are uint16's and
        // lengths other than 2 are strings. Update this code if that
        // assumption changes.
        byte_vector_t element_bytes = byte_vector_t(bytes.begin() + element.second.offset,
            bytes.begin() + element.second.offset + element.second.length);

        mb_eeprom[element.first] = (element.second.length == 2)
                                       ? uint16_bytes_to_string(element_bytes)
                                       : bytes_to_string(element_bytes);
    }

    if (rev > 0) {
        if (mb_eeprom["magic"] != EXPECTED_MAGIC) {
            throw uhd::runtime_error(
                str(boost::format("EEPROM magic value mismatch. Device returns %s, but "
                                  "should have been %s.")
                    % mb_eeprom["magic"] % EXPECTED_MAGIC));
        }
        if (mb_eeprom["eeprom_compat"] != EXPECTED_COMPAT) {
            throw uhd::runtime_error(
                str(boost::format("EEPROM compat value mismatch. Device returns %s, but "
                                  "should have been %s.")
                    % mb_eeprom["eeprom_compat"] % EXPECTED_COMPAT));
        }
    }

    return mb_eeprom;
}

void b200_impl::set_mb_eeprom(const mboard_eeprom_t& mb_eeprom)
{
    const auto rev  = _get_rev(_iface);
    auto eeprom_map = (rev == 0) ? B200_REV0_MAP : B200_REV1_MAP;
    auto base_addr  = (rev == 0) ? REV0_BASE_ADDR : REV1_BASE_ADDR;
    for (const auto& key : mb_eeprom.keys()) {
        if (eeprom_map.find(key) == eeprom_map.end()) {
            UHD_LOG_WARNING(
                LOG_ID, "Unknown key in mb_eeprom during set_mb_eeprom: " << key);
            continue;
        }

        // There is an assumption here that fields of length 2 are uint16's and
        // lengths other than 2 are strings. Update this code if that
        // assumption changes.
        auto field = eeprom_map.at(key);
        auto bytes = (field.length == 2) ? string_to_uint16_bytes(mb_eeprom[key])
                                         : string_to_bytes(mb_eeprom[key], field.length);
        _iface->write_eeprom(base_addr >> 8, (base_addr & 0xFF) + field.offset, bytes);
    }
}
