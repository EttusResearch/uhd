//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/node_accessor.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/complex_gain_3000.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <map>

using namespace uhd;
using namespace uhd::cores;
using namespace uhd::rfnoc;

constexpr size_t NUM_CHANS    = 2;
constexpr size_t NIPC         = 1;
constexpr double TICK_RATE    = 125e6;
constexpr int COEFF_FRAC_BITS = 14;
constexpr int COEFF_WIDTH     = 16;

/* This class extends mock_reg_iface_t by adding a constructor that initializes
 * some of the read/write memory to contain the memory size for the complex gain.
 */
class cgain_mock_reg_iface_t : public mock_reg_iface_t
{
    static constexpr uint32_t chan_offset = 1024;

public:
    cgain_mock_reg_iface_t(size_t num_channels, uhd::time_spec_t& timekeeper)
        : _timekeeper(timekeeper)
    {
        const uint32_t default_gain_value = ((1 << COEFF_FRAC_BITS) << COEFF_WIDTH)
                                            | ((0 << COEFF_FRAC_BITS) & 0x0FFFF);
        for (size_t chan = 0; chan < num_channels; chan++) {
            write_memory[get_addr(chan, 0x00)] = default_gain_value;
        }
    }
    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t time, bool /*ack*/) override
    {
        if (time == uhd::time_spec_t::ASAP || time <= _timekeeper) {
            write_memory[addr] = data;
        }
        return;
    }
    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        read_memory[addr] = write_memory.at(addr);
        return;
    }
    uint32_t get_addr(size_t chan, uint32_t offset)
    {
        return offset + chan * chan_offset;
    }

    uhd::time_spec_t& _timekeeper;
}; // class cgain_mock_reg_iface_t

struct complex_gain_fixture
{
    complex_gain_fixture()
        : reg_iface(std::make_shared<cgain_mock_reg_iface_t>(NUM_CHANS, timekeeper))
        , tx_cgain(std::make_unique<uhd::cores::complex_gain_3000>(
              [&](uint32_t addr,
                  uint32_t data,
                  size_t chan,
                  const uhd::time_spec_t& time) {
                  reg_iface->_poke_cb(reg_iface->get_addr(chan, addr), data, time, false);
              },
              [&](uint32_t addr, size_t chan) {
                  reg_iface->_peek_cb(
                      reg_iface->get_addr(chan, addr), uhd::time_spec_t::ASAP);
                  return reg_iface->read_memory.at(reg_iface->get_addr(chan, addr));
              },
              TICK_RATE,
              uhd::direction_t::TX_DIRECTION,
              NIPC))
        , rx_cgain(std::make_unique<uhd::cores::complex_gain_3000>(
              [&](uint32_t addr,
                  uint32_t data,
                  size_t chan,
                  const uhd::time_spec_t& time) {
                  reg_iface->_poke_cb(reg_iface->get_addr(chan, addr), data, time, false);
              },
              [&](uint32_t addr, size_t chan) {
                  reg_iface->_peek_cb(
                      reg_iface->get_addr(chan, addr), uhd::time_spec_t::ASAP);
                  return reg_iface->read_memory.at(reg_iface->get_addr(chan, addr));
              },
              TICK_RATE,
              uhd::direction_t::RX_DIRECTION,
              NIPC))
    {
    }

    ~complex_gain_fixture(){};

    std::shared_ptr<cgain_mock_reg_iface_t> reg_iface;
    complex_gain_3000::uptr tx_cgain;
    complex_gain_3000::uptr rx_cgain;
    uhd::time_spec_t timekeeper = uhd::time_spec_t(0.0);
};

void advance_timekeeper(uhd::time_spec_t& timekeeper, uint32_t ticks)
{
    timekeeper += uhd::time_spec_t::from_ticks(ticks, TICK_RATE);
}

BOOST_FIXTURE_TEST_CASE(complex_gain_3000_test, complex_gain_fixture)
{
    std::vector<std::complex<double>> gain_list = {std::complex<double>(0.5, 0.5),
        std::complex<double>(-1.5, -1.5),
        std::complex<double>(-1.5, 0.5),
        std::complex<double>(0.5, -1.5)};
    for (std::complex<double> gain : gain_list) {
        const int16_t real_coeff           = gain.real() * (1 << COEFF_FRAC_BITS);
        const int16_t imag_coeff           = gain.imag() * (1 << COEFF_FRAC_BITS);
        const uint32_t expected_gain_value = (real_coeff << COEFF_WIDTH)
                                             | (imag_coeff & 0x0FFFF);
        for (size_t chan = 0; chan < NUM_CHANS; chan++) {
            tx_cgain->set_gain_coeff(gain, chan);
            BOOST_CHECK_EQUAL(reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                expected_gain_value);
            rx_cgain->set_gain_coeff(gain, chan);
            BOOST_CHECK_EQUAL(reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                expected_gain_value);
            BOOST_CHECK_EQUAL(tx_cgain->get_gain_coeff(chan), gain);
            BOOST_CHECK_EQUAL(rx_cgain->get_gain_coeff(chan), gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(timed_complex_gain_test, complex_gain_fixture)
{
    std::vector<std::complex<double>> gain_list = {
        std::complex<double>(0.5, 0.5), std::complex<double>(0.5, -1.5)};
    constexpr uint32_t DEFAULT_GAIN_I = 1;
    constexpr uint32_t DEFAULT_GAIN_Q = 0;
    const uint32_t default_gain_value =
        ((DEFAULT_GAIN_I << COEFF_FRAC_BITS) << COEFF_WIDTH)
        | ((DEFAULT_GAIN_Q << COEFF_FRAC_BITS) & 0x0FFFF);
    for (size_t i = 0; i < gain_list.size(); i++) {
        int16_t real_coeff         = gain_list[i].real() * (1 << COEFF_FRAC_BITS);
        int16_t imag_coeff         = gain_list[i].imag() * (1 << COEFF_FRAC_BITS);
        const uint32_t gain_value  = (real_coeff << COEFF_WIDTH) | (imag_coeff & 0x0FFFF);
        uhd::time_spec_t gain_time = uhd::time_spec_t(1.0 * i);
        std::complex<double> expected_gain = (i > 0) ? gain_list[i - 1] : gain_list[0];
        real_coeff = expected_gain.real() * (1 << COEFF_FRAC_BITS);
        imag_coeff = expected_gain.imag() * (1 << COEFF_FRAC_BITS);
        const uint32_t expected_gain_value = (real_coeff << COEFF_WIDTH)
                                             | (imag_coeff & 0x0FFFF);
        advance_timekeeper(timekeeper, 11);
        for (size_t chan = 0; chan < NUM_CHANS; chan++) {
            tx_cgain->set_gain_coeff(gain_list[i], chan, gain_time);
            if (timekeeper >= gain_time) {
                BOOST_CHECK_EQUAL(
                    reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                    gain_value);
                BOOST_CHECK_EQUAL(tx_cgain->get_gain_coeff(chan), gain_list[i]);
            } else {
                BOOST_CHECK_EQUAL(
                    reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                    (i > 0) ? expected_gain_value : default_gain_value);
                BOOST_CHECK_EQUAL(tx_cgain->get_gain_coeff(chan), expected_gain);
            }
            rx_cgain->set_gain_coeff(gain_list[i], chan, gain_time);
            if (timekeeper >= gain_time) {
                BOOST_CHECK_EQUAL(
                    reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                    gain_value);
                BOOST_CHECK_EQUAL(rx_cgain->get_gain_coeff(chan), gain_list[i]);
            } else {
                BOOST_CHECK_EQUAL(
                    reg_iface->write_memory.at(reg_iface->get_addr(chan, 0x00)),
                    (i > 0) ? expected_gain_value : default_gain_value);
                BOOST_CHECK_EQUAL(rx_cgain->get_gain_coeff(chan), expected_gain);
            }
            advance_timekeeper(timekeeper, 34);
        }
    }
}
