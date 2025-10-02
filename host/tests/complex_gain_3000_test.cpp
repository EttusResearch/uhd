//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include <uhdlib/usrp/cores/complex_gain_3000.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <map>


// FIXME expand testing to RX/TX and to multiple channels, using a fixture
// FIXME check that the time is appropriately moved
BOOST_AUTO_TEST_CASE(complex_gain_3000_tx_test)
{
    uint32_t last_poke_addr = 0;
    std::map<uint32_t, uint32_t> mem;
    uhd::time_spec_t last_poke_time;

    uint32_t last_peek_addr = 0;

    size_t nipc      = 1;
    size_t chan      = 0;
    double tick_rate = 125e6;

    uhd::cores::complex_gain_3000 dut(
        [&](uint32_t addr, uint32_t data, size_t /*chan*/, const uhd::time_spec_t& time) {
            last_poke_addr = addr;
            mem[addr]      = data;
            last_poke_time = time;
            return;
        },
        [&](uint32_t addr, size_t /*chan*/) {
            last_peek_addr = addr;
            if (mem.count(addr) == 0) {
                mem[addr] = 0;
            }

            return mem.at(addr);
        },
        tick_rate,
        uhd::direction_t::TX_DIRECTION,
        nipc);


    std::complex<double> gain{0.5, 0.5};
    dut.set_gain_coeff(gain, chan);


    constexpr int COEFF_FRAC_BITS      = 14;
    constexpr int COEFF_WIDTH          = 16;
    const int16_t real_coeff           = gain.real() * (1 << COEFF_FRAC_BITS);
    const int16_t imag_coeff           = gain.imag() * (1 << COEFF_FRAC_BITS);
    const uint32_t expected_gain_value = (imag_coeff << COEFF_WIDTH) | (real_coeff);

    BOOST_CHECK_EQUAL(mem[0], expected_gain_value);

    std::complex<double> expected{gain};
    BOOST_CHECK_EQUAL(dut.get_gain_coeff(chan), expected);
}
