//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/time_spec.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_cpld_ctrl.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::usrp::zbx;

struct mock_reg_iface_type
{
    uint32_t last_addr = 0;
    zbx_cpld_ctrl::chan_t last_chan;
    std::map<uint32_t, uint32_t> memory;
    uhd::time_spec_t sleep_counter = uhd::time_spec_t(0.0);
};


struct zbx_cpld_fixture
{
    zbx_cpld_fixture()
        : cpld(
            [&](const uint32_t addr,
                const uint32_t data,
                const zbx_cpld_ctrl::chan_t chan) {
                std::cout << "[MOCKREGS] poke32(" << addr << ", " << data << ")"
                          << std::endl;
                mock_reg_iface.last_addr    = addr;
                mock_reg_iface.last_chan    = chan;
                mock_reg_iface.memory[addr] = data;
            },
            [&](const uint32_t addr) -> uint32_t {
                std::cout << "[MOCKREGS] peek32(" << addr << ") => "
                          << mock_reg_iface.memory.at(addr) << std::endl;
                return mock_reg_iface.memory.at(addr);
            },
            [&](const uhd::time_spec_t& time) { mock_reg_iface.sleep_counter += time; },
            "TEST::CPLD")
    {
        // nop
    }

    mock_reg_iface_type mock_reg_iface;
    zbx_cpld_ctrl cpld;
};


BOOST_FIXTURE_TEST_CASE(zbx_cpld_ctrl_test, zbx_cpld_fixture)
{
    cpld.set_scratch(23);
    BOOST_CHECK_EQUAL(cpld.get_scratch(), 23);

    cpld.pulse_lo_sync(0, {zbx_lo_t::TX0_LO1});
    BOOST_CHECK_EQUAL(mock_reg_iface.memory[0x1024], 1);
    mock_reg_iface.memory[0x1024] = 0;
    // Make sure there are no caching issues:
    cpld.pulse_lo_sync(0, {zbx_lo_t::TX0_LO1});
    BOOST_CHECK_EQUAL(mock_reg_iface.memory[0x1024], 1);
    // Now all:
    cpld.pulse_lo_sync(0,
        {zbx_lo_t::TX0_LO1,
            zbx_lo_t::TX0_LO2,
            zbx_lo_t::TX1_LO1,
            zbx_lo_t::TX1_LO2,
            zbx_lo_t::RX0_LO1,
            zbx_lo_t::RX0_LO2,
            zbx_lo_t::RX1_LO1,
            zbx_lo_t::RX1_LO2});
    BOOST_CHECK_EQUAL(mock_reg_iface.memory[0x1024], 0xFF);
    mock_reg_iface.memory[0x1024] = 0;
    cpld.set_lo_sync_bypass(true);
    BOOST_CHECK_THROW(cpld.pulse_lo_sync(0, {zbx_lo_t::TX0_LO1}), uhd::runtime_error);
    BOOST_CHECK_EQUAL(mock_reg_iface.memory[0x1024], 0x100);
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_ant_override_rx_test, zbx_cpld_fixture)
{
    cpld.set_rx_antenna_switches(
        0, uhd::usrp::zbx::ATR_ADDR_RX, uhd::usrp::zbx::ANTENNA_TXRX);

    cpld.set_tx_antenna_switches(
        0, ATR_ADDR_0X, uhd::usrp::zbx::ANTENNA_TXRX, tx_amp::HIGHBAND);
    cpld.set_tx_antenna_switches(
        0, ATR_ADDR_RX, uhd::usrp::zbx::ANTENNA_TXRX, tx_amp::HIGHBAND);
    cpld.set_tx_antenna_switches(
        0, ATR_ADDR_TX, uhd::usrp::zbx::ANTENNA_TXRX, tx_amp::HIGHBAND);
    cpld.set_tx_antenna_switches(
        0, ATR_ADDR_XX, uhd::usrp::zbx::ANTENNA_TXRX, tx_amp::HIGHBAND);

    // Make sure that configuring the TX antenna switches didn't disconnect the RX
    // from the TX/RX port.
    BOOST_CHECK_EQUAL(
        (mock_reg_iface.memory[0x2000 + 4 * uhd::usrp::zbx::ATR_ADDR_RX] >> 20) & 0x3, 0);
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_amp_test, zbx_cpld_fixture)
{
    cpld.set_tx_antenna_switches(
        0, 0, uhd::usrp::zbx::DEFAULT_TX_ANTENNA, tx_amp::HIGHBAND);
    BOOST_CHECK(tx_amp::HIGHBAND == cpld.get_tx_amp_settings(0, 0, false));
    mock_reg_iface.memory[0x2000] = 0;
    BOOST_CHECK(tx_amp::HIGHBAND == cpld.get_tx_amp_settings(0, 0, false));
    BOOST_CHECK(tx_amp::HIGHBAND != cpld.get_tx_amp_settings(0, 0, true));
    BOOST_CHECK(tx_amp::BYPASS == cpld.get_tx_amp_settings(0, 0, false));
}

BOOST_FIXTURE_TEST_CASE(zbx_get_set_dsa_test, zbx_cpld_fixture)
{
    // We only test the first table index
    constexpr size_t dsa_table_index = 0;
    for (const size_t chan : {0, 1}) {
        const uint32_t tx_dsa_table_addr = 0x3000 + chan * 0x400;
        const uint32_t rx_dsa_table_addr = 0x3800 + chan * 0x400;

        auto& tx_dsa_reg = mock_reg_iface.memory[tx_dsa_table_addr];
        auto& rx_dsa_reg = mock_reg_iface.memory[rx_dsa_table_addr];

        // We'll skip DSA3A/B, because they work just like the rest and would
        // make this test much longer and less readable without adding much test
        // coverage.
        for (const auto dsa :
            {zbx_cpld_ctrl::dsa_type::DSA1, zbx_cpld_ctrl::dsa_type::DSA2}) {
            const size_t tx_shift = (dsa == zbx_cpld_ctrl::dsa_type::DSA1) ? 0 : 8;
            const size_t rx_shift = (dsa == zbx_cpld_ctrl::dsa_type::DSA1) ? 0 : 4;
            // 0xB and 0xC are just random attenuation values. They are valid
            // for both TX and RX.
            cpld.set_tx_dsa(chan, dsa_table_index, dsa, 0xB);
            BOOST_CHECK_EQUAL((tx_dsa_reg >> tx_shift) & 0x1F, 0xB);
            tx_dsa_reg = 0x0C0C;
            BOOST_CHECK_EQUAL(0xB, cpld.get_tx_dsa(chan, dsa_table_index, dsa, false));
            BOOST_CHECK_EQUAL(0xC, cpld.get_tx_dsa(chan, dsa_table_index, dsa, true));

            cpld.set_rx_dsa(chan, 0, dsa, 0xB);
            BOOST_CHECK_EQUAL((rx_dsa_reg >> rx_shift) & 0xF, 0xB);
            rx_dsa_reg = 0xCC;
            BOOST_CHECK_EQUAL(0xB, cpld.get_rx_dsa(chan, dsa_table_index, dsa, false));
            BOOST_CHECK_EQUAL(0xC, cpld.get_rx_dsa(chan, dsa_table_index, dsa, true));
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_set_from_table_test, zbx_cpld_fixture)
{
    constexpr uint32_t tx_sel_addr = 0x4000;
    constexpr size_t chan          = 0;
    constexpr uint8_t idx          = 2;

    auto& tx_table_select = mock_reg_iface.memory[tx_sel_addr + idx * 4];
    cpld.set_tx_gain_switches(chan, idx, 23);
    BOOST_REQUIRE_EQUAL(tx_table_select, 23);
}
