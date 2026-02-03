//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/ublox_msg_helper.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(construct_valset_message_single_setting_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    settings[0x10050007] = {0x01}; // CFG_TP_TP1_ENA = 0x01

    uhd::byte_vector_t message = construct_valset_message(3, settings);

    // Expected message structure:
    // Header: 0xB5, 0x62
    // Class/ID: 0x06, 0x8A
    // Length: 0x09, 0x00 (9 bytes: 4 header bytes + 4 key bytes + 1 value byte)
    // Payload: 0x00, 0x03, 0x00, 0x00 (version, layer, reserved)
    // Key: 0x07, 0x00, 0x05, 0x10 (little-endian)
    // Value: 0x01
    // Checksum: 2 bytes

    BOOST_CHECK_EQUAL(message.size(), 17); // 2 + 2 + 2 + 9 + 2
    BOOST_CHECK_EQUAL(message[0], 0xB5); // Header
    BOOST_CHECK_EQUAL(message[1], 0x62);
    BOOST_CHECK_EQUAL(message[2], 0x06); // Class
    BOOST_CHECK_EQUAL(message[3], 0x8A); // ID
    BOOST_CHECK_EQUAL(message[4], 0x09); // Length LSB
    BOOST_CHECK_EQUAL(message[5], 0x00); // Length MSB
    BOOST_CHECK_EQUAL(message[6], 0x00); // Version
    BOOST_CHECK_EQUAL(message[7], 0x03); // Layer
    BOOST_CHECK_EQUAL(message[8], 0x00); // Reserved
    BOOST_CHECK_EQUAL(message[9], 0x00); // Reserved
    BOOST_CHECK_EQUAL(message[10], 0x07); // Key byte 0
    BOOST_CHECK_EQUAL(message[11], 0x00); // Key byte 1
    BOOST_CHECK_EQUAL(message[12], 0x05); // Key byte 2
    BOOST_CHECK_EQUAL(message[13], 0x10); // Key byte 3
    BOOST_CHECK_EQUAL(message[14], 0x01); // Value
}

BOOST_AUTO_TEST_CASE(construct_valset_message_multi_byte_value_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    // CFG_TP_PERIOD_TP1 = 0x000F4240 (1000000 in big-endian)
    settings[0x40050002] = {0x00, 0x0F, 0x42, 0x40};

    uhd::byte_vector_t message = construct_valset_message(3, settings);

    BOOST_CHECK_EQUAL(message.size(), 20); // 2 + 2 + 2 + 12 + 2
    BOOST_CHECK_EQUAL(message[0], 0xB5); // Header
    BOOST_CHECK_EQUAL(message[1], 0x62);
    BOOST_CHECK_EQUAL(message[2], 0x06); // Class
    BOOST_CHECK_EQUAL(message[3], 0x8A); // ID
    BOOST_CHECK_EQUAL(message[4], 0x0C); // Length LSB (12 bytes)
    BOOST_CHECK_EQUAL(message[5], 0x00); // Length MSB
    BOOST_CHECK_EQUAL(message[10], 0x02); // Key byte 0
    BOOST_CHECK_EQUAL(message[11], 0x00); // Key byte 1
    BOOST_CHECK_EQUAL(message[12], 0x05); // Key byte 2
    BOOST_CHECK_EQUAL(message[13], 0x40); // Key byte 3
    // Value should be reversed (little-endian)
    BOOST_CHECK_EQUAL(message[14], 0x40);
    BOOST_CHECK_EQUAL(message[15], 0x42);
    BOOST_CHECK_EQUAL(message[16], 0x0F);
    BOOST_CHECK_EQUAL(message[17], 0x00);
}

BOOST_AUTO_TEST_CASE(construct_valset_message_multiple_settings_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    settings[0x10050007] = {0x01}; // CFG_TP_TP1_ENA
    settings[0x10050009] = {0x01}; // CFG_TP_USE_LOCKED_TP1
    settings[0x1005000a] = {0x01}; // CFG_TP_ALIGN_TO_TOW_TP1

    uhd::byte_vector_t message = construct_valset_message(3, settings);

    // Should have 3 keys with single-byte values
    // Payload: 4 header bytes + (4 key + 1 value) * 3 = 19 bytes
    BOOST_CHECK_EQUAL(message[4], 0x13); // Length LSB (19 bytes)
    BOOST_CHECK_EQUAL(message[5], 0x00); // Length MSB
}

BOOST_AUTO_TEST_CASE(construct_valset_message_too_many_settings_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    // Add 65 settings (exceeds limit of 64)
    for (uint32_t i = 0; i < 65; ++i) {
        settings[i] = {0x01};
    }

    BOOST_CHECK_THROW(construct_valset_message(3, settings), uhd::runtime_error);
}

BOOST_AUTO_TEST_CASE(construct_valget_message_single_key_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    settings[0x10050007] = {}; // CFG_TP_TP1_ENA (value doesn't matter for VALGET)

    uhd::byte_vector_t message = construct_valget_message(0, settings);

    // Expected message structure:
    // Header: 0xB5, 0x62
    // Class/ID: 0x06, 0x8B
    // Length: 0x08, 0x00 (8 bytes: 4 header bytes + 4 key bytes, no value)
    // Payload: 0x00, 0x00, 0x00, 0x00 (version, layer, position, reserved)
    // Key: 0x07, 0x00, 0x05, 0x10 (little-endian)
    // Checksum: 2 bytes

    BOOST_CHECK_EQUAL(message.size(), 16); // 2 + 2 + 2 + 8 + 2
    BOOST_CHECK_EQUAL(message[0], 0xB5); // Header
    BOOST_CHECK_EQUAL(message[1], 0x62);
    BOOST_CHECK_EQUAL(message[2], 0x06); // Class
    BOOST_CHECK_EQUAL(message[3], 0x8B); // ID (VALGET)
    BOOST_CHECK_EQUAL(message[4], 0x08); // Length LSB
    BOOST_CHECK_EQUAL(message[5], 0x00); // Length MSB
    BOOST_CHECK_EQUAL(message[6], 0x00); // Version
    BOOST_CHECK_EQUAL(message[7], 0x00); // Layer
    BOOST_CHECK_EQUAL(message[10], 0x07); // Key byte 0
    BOOST_CHECK_EQUAL(message[11], 0x00); // Key byte 1
    BOOST_CHECK_EQUAL(message[12], 0x05); // Key byte 2
    BOOST_CHECK_EQUAL(message[13], 0x10); // Key byte 3
}

BOOST_AUTO_TEST_CASE(construct_valget_message_multiple_keys_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    settings[0x10050007] = {}; // CFG_TP_TP1_ENA
    settings[0x10050009] = {}; // CFG_TP_USE_LOCKED_TP1

    uhd::byte_vector_t message = construct_valget_message(0, settings);

    // Payload: 4 header bytes + 4 key bytes * 2 = 12 bytes
    BOOST_CHECK_EQUAL(message[4], 0x0C); // Length LSB (12 bytes)
    BOOST_CHECK_EQUAL(message[5], 0x00); // Length MSB
}

BOOST_AUTO_TEST_CASE(construct_valget_message_too_many_keys_test)
{
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    // Add 65 keys (exceeds limit of 64)
    for (uint32_t i = 0; i < 65; ++i) {
        settings[i] = {};
    }

    BOOST_CHECK_THROW(construct_valget_message(0, settings), uhd::runtime_error);
}

BOOST_AUTO_TEST_CASE(construct_valset_message_checksum_test)
{
    // Test that checksum is properly calculated using Fletcher algorithm
    uhd::dict<uint32_t, uhd::byte_vector_t> settings;
    settings[0x10050007] = {0x01};

    uhd::byte_vector_t message = construct_valset_message(3, settings);

    // Manually calculate Fletcher checksum for verification
    // Checksum is calculated over Class/ID + Length + Payload
    uint8_t ck_a = 0;
    uint8_t ck_b = 0;
    for (size_t i = 2; i < message.size() - 2; ++i) {
        ck_a += message[i];
        ck_b += ck_a;
    }

    BOOST_CHECK_EQUAL(message[message.size() - 2], ck_a);
    BOOST_CHECK_EQUAL(message[message.size() - 1], ck_b);
}
