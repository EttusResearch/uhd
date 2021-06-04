//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <boost/test/unit_test.hpp>

using uhd::rfnoc::x400::rfdc_control;

struct x400_rfdc_fixture
{
    static constexpr size_t RFDC_MEM_SIZE = 1;

    x400_rfdc_fixture()
        : mem(1 << rfdc_control::regmap::NCO_RESET_DONE_MSB, RFDC_MEM_SIZE)
        , rfdcc(
              uhd::memmap32_iface_timed{
                  [&](const uint32_t addr, const uint32_t data, const uhd::time_spec_t&) {
                      mem[addr] = data;
                  },
                  [&](const uint32_t addr) { return mem[addr]; }},
              "TEST::RFDC")
    {
        // nop
    }

    std::vector<uint32_t> mem;
    rfdc_control rfdcc;
};


BOOST_FIXTURE_TEST_CASE(test_nco_reset, x400_rfdc_fixture)
{
    rfdcc.reset_ncos({rfdc_control::rfdc_type::RX0}, uhd::time_spec_t::ASAP);
    BOOST_CHECK(mem[rfdc_control::regmap::NCO_RESET] & 0x1);
    // Fake self-clearing bit
    mem[rfdc_control::regmap::NCO_RESET] = 1 << rfdc_control::regmap::NCO_RESET_DONE_MSB;
    // This should print a warning:
    rfdcc.reset_ncos({}, uhd::time_spec_t::ASAP);
    BOOST_CHECK_EQUAL(mem[rfdc_control::regmap::NCO_RESET]
                          & (1 << rfdc_control::regmap::NCO_RESET_START_MSB),
        0);
    BOOST_CHECK(rfdcc.get_nco_reset_done());
}

BOOST_FIXTURE_TEST_CASE(test_nco_freq, x400_rfdc_fixture)
{
    // TODO: Add checks when implemented
    rfdcc.set_nco_freq(rfdc_control::rfdc_type::RX0, 1e9);
}
