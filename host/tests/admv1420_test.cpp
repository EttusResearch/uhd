//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "admv1420_regs.hpp"
#include <uhdlib/usrp/common/admv1420.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <set>

class admv1420_mem
{
public:
    admv1420_mem()
    {
        // Copy default values on the HW into mock memory object
        for (const uint16_t addr : regs.get_all_addrs<uint16_t>(true)) {
            mem[addr] = regs.get_reg(addr);
        }
    }

    void poke16(const uint16_t addr, const uint16_t data)
    {
        if (!regs.get_all_addrs<uint16_t>(true).count(addr)) {
            throw uhd::runtime_error("Register does not exist!");
        }
        if (!regs.get_all_addrs<uint16_t>(false).count(addr)) {
            throw uhd::runtime_error("Writing to a read-only register!");
        }
        mem[addr] = data;
    }

    uint16_t peek16(const uint16_t addr)
    {
        return mem.at(addr);
    }

    admv1420_regs_t regs;
    std::map<uint16_t, uint16_t> mem;
};

void UHD_CHECK_REGMAP(
    std::map<uint16_t, uint16_t> expected, std::map<uint16_t, uint16_t> actual)
{
    for (const auto& expected_reg : expected) {
        // Create strings for expected and actual values for better debugging
        const std::string exp_str = "R" + std::to_string(expected_reg.first)
                                    + "==" + std::to_string(expected_reg.second);
        const std::string act_str = "R" + std::to_string(expected_reg.first) + "=="
                                    + std::to_string(actual.at(expected_reg.first));
        BOOST_CHECK_EQUAL(exp_str, act_str);
    }
}

// This is a test for verifying successfull initialization of the admv1420_regs
BOOST_AUTO_TEST_CASE(admv1420_init_test)
{
    auto mem      = admv1420_mem{}; // Mock memory for the ADMV1420
    auto admv1420 = admv1420_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); },
        "ADMV1420_TEST");

    // In ADMV1420 we reset first (reg 0x0 = 0x81), but since those registers are latching
    // in HW, we pull them down again in SW but don't actively write that to HW again.
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x0, 0x81},
        },
        mem.mem);
}

// This tests if the set_rf_band function works as expected in the admv1420 ctrl
BOOST_AUTO_TEST_CASE(admv1420_set_rf_band_test)
{
    auto mem      = admv1420_mem{}; // Mock memory for the ADMV1420
    auto admv1420 = admv1420_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); },
        "ADMV1420_TEST");


    admv1420->set_rf_band(admv1420_iface::rf_band_t::BAND_2);
    admv1420->commit();

    // Verify that the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x800, 0xB0}, // Register 0x800 should hold the RF band value
        },
        mem.mem);

    // Test setting another RF band value
    admv1420->set_rf_band(admv1420_iface::rf_band_t::BAND_3);
    admv1420->commit();

    // Verify the updated register values
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x800, 0xF0}, // Register 0x800 should now hold the new RF band value
        },
        mem.mem);
}

BOOST_AUTO_TEST_CASE(admv1420_set_dsa_test)
{
    auto mem      = admv1420_mem{}; // Mock memory for the ADMV1420
    auto admv1420 = admv1420_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); },
        "ADMV1420_TEST");

    // Test setting DSA1 to a valid value
    uint8_t result = admv1420->set_dsa(admv1420_iface::dsa_t::DSA1, 10);
    admv1420->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28B, 0x0A}, // Register 0x28B holds the DSA1 value
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 10); // Ensure the returned value matches the input

    // Test setting DSA2 to a valid value (rounded to either 0 or 6)
    result = admv1420->set_dsa(admv1420_iface::dsa_t::DSA2, 5);
    admv1420->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28B, 0x1A},
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 6); // Ensure the returned value is rounded to 6

    // Test setting DSA3 to a valid value
    result = admv1420->set_dsa(admv1420_iface::dsa_t::DSA3, 15);
    admv1420->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28C, 0x0F},
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 15);

    // Test setting DSA4 to a valid value
    result = admv1420->set_dsa(admv1420_iface::dsa_t::DSA4, 7);
    admv1420->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28C, 0x7F},
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 7);

    // Test setting DSA5 to a valid value
    result = admv1420->set_dsa(admv1420_iface::dsa_t::DSA5, 12);
    admv1420->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28D, 0xC},
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 12);

    // Test invalid value for DSA2 (greater than 6)
    BOOST_CHECK_THROW(
        admv1420->set_dsa(admv1420_iface::dsa_t::DSA2, 7), uhd::assertion_error);

    // Test invalid value for DSA1 (greater than 15)
    BOOST_CHECK_THROW(
        admv1420->set_dsa(admv1420_iface::dsa_t::DSA1, 16), uhd::assertion_error);
}
