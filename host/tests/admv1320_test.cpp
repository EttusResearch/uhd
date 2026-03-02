//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "admv1320_regs.hpp"
#include <uhdlib/usrp/common/admv1320.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <set>

class admv1320_mem
{
public:
    admv1320_mem()
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

    admv1320_regs_t regs;
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

// This is a test for verifying successful initialization of the admv1320_regs
BOOST_AUTO_TEST_CASE(admv1320_init_test)
{
    auto mem      = admv1320_mem{}; // Mock memory for the ADMV1320
    auto admv1320 = admv1320_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); });

    // Check against default register values
    // In ADMV1320 we reset first (reg 0x0 = 0x81), but since those registers are latching
    // in HW, we pull them down again in SW. Then we set SDO_ACTIVE which is reg 0x0, too.
    // So in the end only the two SDO_ACTIVE bits remain set which leads to 0x18 in reg
    // 0x0.
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x0, 0x18},
        },
        mem.mem);
}

// This tests if the set_rf_band function works as expected in the admv1320 control
BOOST_AUTO_TEST_CASE(admv1320_set_rf_band_test)
{
    auto mem      = admv1320_mem{}; // Mock memory for the ADMV1320
    auto admv1320 = admv1320_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); });

    admv1320->set_rf_band(admv1320_iface::rf_band_t::BAND_2);
    admv1320->commit();

    // Verify that the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x800, 0x40}, // Register 0x800 should hold the RF band value
        },
        mem.mem);

    // Test setting another RF band value
    admv1320->set_rf_band(admv1320_iface::rf_band_t::BAND_3);
    admv1320->commit();

    // Verify the updated register values
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x800, 0x60}, // Register 0x800 should now hold the new RF band value
        },
        mem.mem);
}

BOOST_AUTO_TEST_CASE(admv1320_set_dsa_test)
{
    auto mem      = admv1320_mem{}; // Mock memory for the ADMV1320
    auto admv1320 = admv1320_iface::make(
        [&](const uint16_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint16_t addr) -> uint16_t { return mem.peek16(addr); });

    // Test setting DSA1 to a valid value
    uint8_t result = admv1320->set_dsa(admv1320_iface::dsa_t::DSA1, 10);
    admv1320->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28B, 0x0A}, // Register 0x28B holds the DSA1 value
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 10); // Ensure the returned value matches the input

    // Test setting DSA2 to a valid value
    result = admv1320->set_dsa(admv1320_iface::dsa_t::DSA2, 7);
    admv1320->commit();

    // Verify the correct register values were written
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28B, 0x7A},
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 7);

    // Test setting DSA1 to a value greater than the maximum (15)
    // The value should be clamped to 15
    result = admv1320->set_dsa(admv1320_iface::dsa_t::DSA1, 19);
    admv1320->commit();

    // Verify the register value is clamped to 15
    UHD_CHECK_REGMAP(
        std::map<uint16_t, uint16_t>{
            {0x28B, 0x7F}, // Register 0x28B should hold the clamped DSA1 value
        },
        mem.mem);
    BOOST_CHECK_EQUAL(result, 15); // Ensure the returned value is clamped to 15
}
