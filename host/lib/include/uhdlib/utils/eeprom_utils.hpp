//
// Copyright 2017-18 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/byte_vector.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/utils/log.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <string>
#include <vector>

static const size_t SERIAL_LEN   = 9;
static const size_t NAME_MAX_LEN = 32 - SERIAL_LEN;

//! convert a string to a byte vector to write to eeprom
uhd::byte_vector_t string_to_uint16_bytes(const std::string& num_str);

//! convert a byte vector read from eeprom to a string
std::string uint16_bytes_to_string(const uhd::byte_vector_t& bytes);

/*!
 * Check for duplicate values within a given set of keys.  Assumes the desire
 * is to replace current EEPROM contents with new EEPROM contents and checks to
 * see if the resulting contents will contain duplicates.  Useful error
 * messages are logged describing any duplicates.
 *
 * <field_type> must provide to_string() and from_string() functions
 *
 * \param error_label Label to put on error messages
 * \param new_eeprom New EEPROM contents
 * \param curr_eeprom Current EEPROM contents
 * \param category Category label for the type of values being checked
 * \param keys Keys to examine for duplicate values
 * \return true if duplicates are found, false if not
 */
template <typename field_type>
bool check_for_duplicates(const std::string& error_label,
    const uhd::dict<std::string, std::string>& new_eeprom,
    const uhd::dict<std::string, std::string>& curr_eeprom,
    const std::string& category,
    const std::vector<std::string>& keys)
{
    bool has_duplicates = false;
    for (size_t i = 0; i < keys.size(); i++) {
        bool found_duplicate = false;
        auto key             = keys[i];

        if (not new_eeprom.has_key(key)) {
            continue;
        }

        auto value = field_type::from_string(new_eeprom[key]).to_string();

        // Check other values in new_eeprom for duplicate
        // Starting at key index i+1 so the same duplicate is not found twice
        for (size_t j = i + 1; j < keys.size(); j++) {
            auto other_key = keys[j];
            if (not new_eeprom.has_key(other_key)) {
                continue;
            }
            auto other_value = field_type::from_string(new_eeprom[other_key]).to_string();
            if (value == other_value) {
                // Value is a duplicate of another supplied value
                UHD_LOG_ERROR(error_label,
                    "Duplicate " << category << " " << new_eeprom[key]
                                 << " is supplied for both " << key << " and "
                                 << other_key);
                found_duplicate = true;
            }
        }
        // Check all keys in curr_eeprom for duplicate value
        for (auto other_key : keys) {
            // Skip any keys in new_eeprom
            if (new_eeprom.has_key(other_key)) {
                continue;
            }

            if (value == curr_eeprom[other_key]) {
                // Value is duplicate of one in the EEPROM
                UHD_LOG_ERROR(error_label,
                    "Duplicate " << category << " " << new_eeprom[key]
                                 << " is already in use for " << other_key);
                found_duplicate = true;
            }
        }
        has_duplicates |= found_duplicate;
    }
    return has_duplicates;
}
