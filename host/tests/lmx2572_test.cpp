//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "lmx2572_regs.hpp"
#include <uhdlib/usrp/common/lmx2572.hpp>
#include <boost/test/unit_test.hpp>
#include <map>


class lmx2572_mem
{
public:
    lmx2572_mem()
    {
        // Copy silicone defaults into mem
        for (uint8_t addr = 0; addr < regs.get_num_regs(); addr++) {
            mem[addr] = regs.get_reg(addr);
        }
    }

    void poke16(const uint8_t addr, const uint16_t data)
    {
        if (regs.get_ro_regs().count(addr)) {
            throw uhd::runtime_error("Writing to RO reg!");
        }
        mem[addr] = data;
    }

    uint16_t peek16(const uint8_t addr)
    {
        return mem.at(addr);
    }

    lmx2572_regs_t regs;
    std::map<uint8_t, uint16_t> mem;
};


BOOST_AUTO_TEST_CASE(lmx_init_test)
{
    auto mem = lmx2572_mem{};
    auto lo  = lmx2572_iface::make(
        [&](const uint8_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint8_t addr) -> uint16_t { return mem.peek16(addr); },
        [](const uhd::time_spec_t&) {});
    lo->reset();
}

void UHD_CHECK_REGMAP(
    std::map<uint8_t, uint16_t> expected, std::map<uint8_t, uint16_t> actual)
{
    for (const auto& expected_r : expected) {
        // Little hack so if this fails, we see all the info:
        const std::string exp_str = "R" + std::to_string(expected_r.first)
                                    + "==" + std::to_string(expected_r.second);
        const std::string act_str = "R" + std::to_string(expected_r.first)
                                    + "==" + std::to_string(actual.at(expected_r.first));
        BOOST_CHECK_EQUAL(exp_str, act_str);
    }
}

BOOST_AUTO_TEST_CASE(lmx_sync_tune_test)
{
    auto mem = lmx2572_mem{};
    auto lo  = lmx2572_iface::make(
        [&](const uint8_t addr, const uint16_t data) { mem.poke16(addr, data); },
        [&](const uint8_t addr) -> uint16_t { return mem.peek16(addr); },
        [](const uhd::time_spec_t&) {});
    lo->reset();
    // Mimick ZBX settings:
    constexpr bool zbx_spur_dodging = false;
    lo->set_sync_mode(true);
    lo->set_output_enable(lmx2572_iface::output_t::RF_OUTPUT_A, true);
    lo->set_output_enable(lmx2572_iface::output_t::RF_OUTPUT_B, false);
    // Test Category 1A + SYNC:
    lo->set_frequency(50 * 64e6, 64e6, zbx_spur_dodging);
    lo->commit();
    // These values are generated with TICS PRO. We don't check all the values,
    // mainly the ones related to sync operation.
    UHD_CHECK_REGMAP(
        std::map<uint8_t, uint16_t>{
            {36, 0x0032}, // Lower bits of N-divider, integer part
            {42, 0x0000}, // PLL_NUM upper
            {43, 0x0000}, // PLL_NUM lower
        },
        mem.mem);
    // Test max frequency just to test boundary conditions:
    lo->set_frequency(100 * 64e6, 64e6, zbx_spur_dodging);
    lo->commit();

    // Test Category 1B + SYNC:
    // Will set CHDIV to 2.
    lo->set_frequency(40 * 64e6, 64e6, zbx_spur_dodging);
    lo->commit();
    UHD_CHECK_REGMAP(
        std::map<uint8_t, uint16_t>{
            {36, 0x0028},
            {42, 0x0000},
            {43, 0x0000},
        },
        mem.mem);

    // Test Category 2 + SYNC:
    // Will set CHDIV to 8.
    lo->set_frequency(10 * 64e6, 64e6, zbx_spur_dodging);
    lo->commit();
    UHD_CHECK_REGMAP(
        std::map<uint8_t, uint16_t>{
            {36, 0x0050},
            {42, 0x0000},
            {43, 0x0000},
        },
        mem.mem);
    // VCO_PHASE_SYNC_EN must be off in this case, b/c we're using the SYNC pin
    BOOST_CHECK_EQUAL(mem.mem[0] & (1 << 14), 0);

    // Test Category 3 + SYNC:
    // Will set CHDIV to 1.
    lo->set_frequency(50.5 * 64e6, 64e6, zbx_spur_dodging);
    lo->commit();
    UHD_CHECK_REGMAP(
        std::map<uint8_t, uint16_t>{
            {36, 0x0032},
        },
        mem.mem);
    // VCO_PHASE_SYNC_EN must be on in this case
    BOOST_CHECK(mem.mem[0] & (1 << 14));

    // Will set CHDIV to 2.
    lo->set_frequency(50.5 * 64e6 / 2, 64e6, zbx_spur_dodging);
    lo->commit();
    UHD_CHECK_REGMAP(
        std::map<uint8_t, uint16_t>{
            {11, 0xB028}, // PLL_R == 2. Note this is a ZBX-specific design choice.
            {36, 0x0032}, // With PLL_R == 2, you would expect this to be 100, but it's
                          // only half that!
        },
        mem.mem);
    // VCO_PHASE_SYNC_EN must be on in this case
    BOOST_CHECK(mem.mem[0] & (1 << 14));
}
