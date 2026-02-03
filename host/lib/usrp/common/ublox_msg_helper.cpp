//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/ublox_msg_helper.hpp>

namespace {
// Calculate the UBLOX checksum, uses the 8-Bit Fletcher Algorithm per Ublox
// documentation.
uhd::byte_vector_t calculate_ublox_checksum(const uhd::byte_vector_t& data)
{
    uhd::byte_vector_t checksum = {0x0, 0x0};
    for (uint16_t i = 0; i < data.size(); i++) {
        checksum[0] = checksum[0] + data[i];
        checksum[1] = checksum[1] + checksum[0];
    }
    return checksum;
}
} // namespace

uhd::byte_vector_t construct_valset_message(
    uint8_t cfg_layer, uhd::dict<uint32_t, uhd::byte_vector_t> settings)
{
    if (settings.size() > 64) {
        throw uhd::runtime_error("UBLOX VALSET messages limited to 64 settings!");
    }
    uhd::byte_vector_t message = UBLOX_HDR;

    // Start constructing the byte vector which will be used to calculate the checksum
    // bytes.
    uhd::byte_vector_t checksum_byte_range = UBLOX_CFG_VALSET_ID;

    // Add payload
    uhd::byte_vector_t payload;
    payload.push_back(0x00);
    payload.push_back(cfg_layer);
    payload.push_back(0x00);
    payload.push_back(0x00);
    for (const auto& key : settings.keys()) {
        // Add key (convert to little-endian)
        payload.push_back(static_cast<uint8_t>(key & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 16) & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 24) & 0xFF));

        // Add value bytes (assumes passed in big-endian, converts to little-endian)
        const uhd::byte_vector_t& value = settings.get(key);
        payload.insert(payload.end(), value.rbegin(), value.rend());
    }

    // Add 2-byte length of the payload
    uint16_t length = payload.size();
    checksum_byte_range.push_back(static_cast<uint8_t>(length & 0xFF));
    checksum_byte_range.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));

    checksum_byte_range.insert(checksum_byte_range.end(), payload.begin(), payload.end());

    uhd::byte_vector_t checksum = calculate_ublox_checksum(checksum_byte_range);

    message.insert(message.end(), checksum_byte_range.begin(), checksum_byte_range.end());
    message.insert(message.end(), checksum.begin(), checksum.end());

    return message;
}

uhd::byte_vector_t construct_valget_message(
    uint8_t cfg_layer, uhd::dict<uint32_t, uhd::byte_vector_t> settings)
{
    if (settings.size() > 64) {
        throw uhd::runtime_error("UBLOX VALGET messages limited to 64 settings!");
    }
    uhd::byte_vector_t message = UBLOX_HDR;

    // Start constructing the byte vector which will be used to calculate the checksum
    // bytes.
    uhd::byte_vector_t checksum_byte_range = UBLOX_CFG_VALGET_ID;

    // Add payload
    uhd::byte_vector_t payload;
    payload.push_back(0x00);
    payload.push_back(cfg_layer);
    payload.push_back(0x00);
    payload.push_back(0x00);
    for (const auto& key : settings.keys()) {
        // Add key (convert to little-endian)
        payload.push_back(static_cast<uint8_t>(key & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 16) & 0xFF));
        payload.push_back(static_cast<uint8_t>((key >> 24) & 0xFF));
    }

    // Add 2-byte length of the payload
    uint16_t length = payload.size();
    checksum_byte_range.push_back(static_cast<uint8_t>(length & 0xFF));
    checksum_byte_range.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));

    checksum_byte_range.insert(checksum_byte_range.end(), payload.begin(), payload.end());

    uhd::byte_vector_t checksum = calculate_ublox_checksum(checksum_byte_range);

    message.insert(message.end(), checksum_byte_range.begin(), checksum_byte_range.end());
    message.insert(message.end(), checksum.begin(), checksum.end());

    return message;
}
