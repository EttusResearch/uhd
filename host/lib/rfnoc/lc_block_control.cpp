//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/lc_block_control.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <algorithm>
#include <regex>
#include <set>

using namespace uhd::rfnoc;

namespace {

const std::regex KEY_RE             = std::regex("^([A-Z2-7]{5}-){11}([A-Z2-7]{5})$");
constexpr size_t BITS_PER_BYTE      = 8;
constexpr size_t BITS_PER_B32_BLOCK = 5;
constexpr size_t BINARY_KEY_LEN     = 37;
constexpr size_t BASE32_KEY_LEN     = 60; // Without padding characters!

bool validate_key(const std::string& key)
{
    std::smatch matches;
    return std::regex_match(key, matches, KEY_RE);
}

//! Maps A to 0, Z to 25, '2' to 26 and so on.
// Assumes c is in valid range!
int base32_get_char_index(uint8_t c)
{
    constexpr uint8_t A      = static_cast<uint8_t>('A');
    constexpr uint8_t TWO    = static_cast<uint8_t>('2');
    constexpr int TWO_OFFSET = 26;
    return c < A ? (c - TWO + TWO_OFFSET) : (c - A);
}

//! Reformat the key from AAAAA-... to remove dashes
std::string format_base32_key(const std::string& user_key)
{
    std::string b32_key{};
    std::copy_if(
        user_key.begin(), user_key.end(), std::back_inserter(b32_key), [](char c) {
            return c != '-';
        });
    return b32_key;
}


//! Do base32 decoding
//
// This function assumes the input is valid base32 and of the right length
std::vector<uint8_t> base32_decode(const std::string& encoded_key)
{
    std::vector<uint8_t> decoded_data(BINARY_KEY_LEN, 0);

    uint8_t mask, current_byte = 0;
    size_t bits_left = 8;
    for (size_t i = 0, j = 0; i < BASE32_KEY_LEN; i++) {
        int char_index = base32_get_char_index((uint8_t)encoded_key[i]);
        if (bits_left > BITS_PER_B32_BLOCK) {
            mask         = (uint8_t)char_index << (bits_left - BITS_PER_B32_BLOCK);
            current_byte = (uint8_t)(current_byte | mask);
            bits_left -= BITS_PER_B32_BLOCK;
        } else {
            mask              = (uint8_t)char_index >> (BITS_PER_B32_BLOCK - bits_left);
            current_byte      = (uint8_t)(current_byte | mask);
            decoded_data[j++] = current_byte;
            current_byte =
                (uint8_t)(char_index << (BITS_PER_BYTE - BITS_PER_B32_BLOCK + bits_left));
            bits_left += BITS_PER_BYTE - BITS_PER_B32_BLOCK;
        }
    }
    return decoded_data;
}

std::vector<uint32_t> repack_key(const std::vector<uint8_t>& binary_key)
{
    std::vector<uint32_t> key_words;
    key_words.reserve(9);
    for (size_t i = 1; i < BINARY_KEY_LEN; i += 4) {
        const uint32_t* key_word_raw =
            reinterpret_cast<const uint32_t*>(binary_key.data() + i);
        key_words.push_back(uhd::byteswap(*key_word_raw));
    }
    return key_words;
}
} // namespace

const uint32_t lc_block_control::REG_COMPAT_NUM        = 0x00;
const uint32_t lc_block_control::REG_FEATURE_ID        = 0x04;
const uint32_t lc_block_control::REG_USER_KEY          = 0x08;
const uint32_t lc_block_control::REG_FEATURE_ENABLE_RB = 0x0C;
const uint32_t lc_block_control::REG_FEATURE_LIST_RB   = 0x10;

const uint16_t lc_block_control::MAJOR_COMPAT = 0;
const uint16_t lc_block_control::MINOR_COMPAT = 0;

class lc_block_control_impl : public lc_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(lc_block_control), _fpga_compat(regs().peek32(REG_COMPAT_NUM))
    {
        constexpr size_t MAX_NUM_FIDS = 100;

        // Load feature IDs until we get a duplicate
        for (size_t i = 0; i < MAX_NUM_FIDS; ++i) {
            _fids.insert(regs().peek32(REG_FEATURE_LIST_RB));
            if (_fids.size() - 1 < i) {
                break;
            }
        }
        if (!_fids.count(regs().peek32(REG_FEATURE_LIST_RB))) {
            RFNOC_LOG_WARNING("License checker block exceeds max number of features!");
        }
    }

    bool load_key(const std::string& key) final
    {
        if (!validate_key(key)) {
            RFNOC_LOG_WARNING("Invalid license key format!");
            return false;
        }
        const auto binary_key = base32_decode(format_base32_key(key));
        if (binary_key[0] != 1) {
            RFNOC_LOG_WARNING("Invalid license key format!");
            return false;
        }
        const auto packed_key = repack_key(binary_key);
        if (!_fids.count(packed_key[0])) {
            RFNOC_LOG_WARNING("This key is valid, but does not match any feature "
                              "available on this device: "
                              << key);
            return false;
        }
        RFNOC_LOG_TRACE("Loading license key " << key);
        RFNOC_LOG_DEBUG(
            "Loading license key for feature " << std::hex << packed_key[0] << std::dec);
        regs().poke32(REG_FEATURE_ID, packed_key[0]);
        for (size_t i = 1; i < packed_key.size(); ++i) {
            regs().poke32(REG_USER_KEY, packed_key[i]);
        }
        const uint32_t success = regs().peek32(REG_FEATURE_ENABLE_RB);
        if (!((success >> 31) & 0x1)) {
            RFNOC_LOG_INFO("A license key was loaded, but not recognized: " << key);
            return false;
        }
        RFNOC_LOG_DEBUG("License was accepted.");
        return true;
    }

    std::vector<uint32_t> get_feature_ids() override
    {
        return {_fids.cbegin(), _fids.cend()};
    }

private:
    uhd::compat_num32 _fpga_compat;

    //! Stores a set of feature IDs this license checker block is managing
    std::set<uint32_t> _fids{};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    lc_block_control, LICCHECK_BLOCK, "LicenseCheck", CLOCK_KEY_GRAPH, "bus_clk")
