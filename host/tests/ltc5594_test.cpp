//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhdlib/usrp/common/ltc5594.hpp>
#include <boost/test/unit_test.hpp>
#include <array>

BOOST_AUTO_TEST_CASE(ltc5594_lo_matching_test)
{
    std::array<uint8_t, 24> mock_registers{};
    auto mock_poke = [&mock_registers](
                         uint8_t addr, uint8_t data) { mock_registers.at(addr) = data; };
    auto mock_peek = [&mock_registers](uint8_t addr) { return mock_registers.at(addr); };

    auto ltc5594 =
        ltc5594_iface::make(std::move(mock_poke), std::move(mock_peek), "ltc5594_test");
    BOOST_REQUIRE(ltc5594);

    ltc5594->set_lo_matching(500e6);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[18]), 0x51U);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[19]), 0x5fU);

    ltc5594->set_lo_matching(100e6);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[18]), 0x48U);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[19]), 0xe3U);

    ltc5594->set_lo_matching(9.1e9);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[18]), 0x48U);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(mock_registers[19]), 0xe3U);
}