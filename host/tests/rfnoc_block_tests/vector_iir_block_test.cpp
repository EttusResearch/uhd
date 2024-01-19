//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhd/rfnoc/vector_iir_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

constexpr size_t NUM_CHANS       = 4;
constexpr uint16_t MAX_DELAY_LEN = 2047;
constexpr size_t DEFAULT_MTU     = 8000;
constexpr double DEFAULT_ALPHA   = 0.9;
constexpr double DEFAULT_BETA    = 0.9;
constexpr uint16_t DEFAULT_DELAY = MAX_DELAY_LEN;

/*
 * This class extends mock_reg_iface_t, handling three particular registers
 */
class vector_iir_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    vector_iir_mock_reg_iface_t(size_t num_chans, uint16_t max_delay)
    {
        for (size_t chan = 0; chan < num_chans; chan++) {
            _max_delays.push_back(max_delay);
            _delays.push_back(0);
        }
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        size_t chan   = addr / vector_iir_block_control::REG_BLOCK_SIZE;
        size_t offset = addr % vector_iir_block_control::REG_BLOCK_SIZE;
        if (offset == vector_iir_block_control::REG_DELAY_OFFSET) {
            _delays[chan] = static_cast<uint16_t>(data & 0xffff);
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        size_t chan   = addr / vector_iir_block_control::REG_BLOCK_SIZE;
        size_t offset = addr % vector_iir_block_control::REG_BLOCK_SIZE;
        if (offset == vector_iir_block_control::REG_DELAY_OFFSET) {
            read_memory[addr] = (static_cast<uint32_t>(_max_delays.at(chan)) << 16)
                                | _delays.at(chan);
        }
    }

private:
    std::vector<uint16_t> _max_delays;
    std::vector<uint16_t> _delays;
};

/*
 * vector_iir_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and vector_iir_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */
struct vector_iir_block_fixture
{
    vector_iir_block_fixture()
        : reg_iface(
            std::make_shared<vector_iir_mock_reg_iface_t>(NUM_CHANS, MAX_DELAY_LEN))
        , block_container(get_mock_block(VECTOR_IIR_BLOCK,
              NUM_CHANS,
              NUM_CHANS,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_vector_iir(block_container.get_block<vector_iir_block_control>())
    {
        node_accessor.init_props(test_vector_iir.get());
    }

    inline uint32_t calculate_alphabeta_register_value(double alphabeta) const
    {
        return uint32_t(alphabeta * pow(2, 31));
    }

    inline size_t calculate_register_address(size_t offset, size_t channel) const
    {
        return channel * vector_iir_block_control::REG_BLOCK_SIZE + offset;
    }

    inline uint32_t check_poke_alpha(size_t channel) const
    {
        return reg_iface->write_memory.at(calculate_register_address(
            vector_iir_block_control::REG_ALPHA_OFFSET, channel));
    }

    inline uint32_t check_poke_beta(size_t channel) const
    {
        return reg_iface->write_memory.at(calculate_register_address(
            vector_iir_block_control::REG_BETA_OFFSET, channel));
    }

    inline uint16_t check_poke_delay(size_t channel) const
    {
        return reg_iface->write_memory.at(calculate_register_address(
                   vector_iir_block_control::REG_DELAY_OFFSET, channel))
               & 0xffff;
    }

    inline uint16_t check_peek_max_delay(size_t channel) const
    {
        return static_cast<uint16_t>(
            reg_iface->read_memory.at(calculate_register_address(
                vector_iir_block_control::REG_DELAY_OFFSET, channel))
            >> 16);
    }

    std::shared_ptr<vector_iir_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<vector_iir_block_control> test_vector_iir;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the vector_iir block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(vector_iir_test_construction, vector_iir_block_fixture)
{
    // Check that the registers were written with their expected initial
    // values.
    const uint32_t default_alpha_value =
        calculate_alphabeta_register_value(DEFAULT_ALPHA);
    const uint32_t default_beta_value = calculate_alphabeta_register_value(DEFAULT_BETA);
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        BOOST_CHECK_EQUAL(check_poke_alpha(chan), default_alpha_value);
        BOOST_CHECK_EQUAL(check_poke_beta(chan), default_beta_value);
        BOOST_CHECK_EQUAL(check_poke_delay(chan), DEFAULT_DELAY);
        BOOST_CHECK_EQUAL(check_peek_max_delay(chan), MAX_DELAY_LEN);
    }
}

/*
 * This test case exercises the API and ensures that the registers are
 * programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(vector_iir_test_api, vector_iir_block_fixture)
{
    const uint32_t default_beta_value = calculate_alphabeta_register_value(DEFAULT_BETA);
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        // Set alpha; ensure beta and delay remain unchanged
        const double alpha = 0.25 + (0.1 * chan);
        test_vector_iir->set_alpha(alpha, chan);
        BOOST_CHECK_EQUAL(test_vector_iir->get_alpha(chan), alpha);
        BOOST_CHECK_EQUAL(
            check_poke_alpha(chan), calculate_alphabeta_register_value(alpha));
        BOOST_CHECK_EQUAL(check_poke_beta(chan), default_beta_value);

        // Set beta; ensure alpha and delay remain unchanged
        const double beta = 0.6 - (0.1 * chan);
        test_vector_iir->set_beta(beta, chan);
        BOOST_CHECK_EQUAL(test_vector_iir->get_beta(chan), beta);
        BOOST_CHECK_EQUAL(
            check_poke_beta(chan), calculate_alphabeta_register_value(beta));
        BOOST_CHECK_EQUAL(
            check_poke_alpha(chan), calculate_alphabeta_register_value(alpha));

        // Set delay; ensure alpha and beta remain unchanged
        const uint16_t delay = 500 + (100 * chan);
        test_vector_iir->set_delay(delay, chan);
        BOOST_CHECK_EQUAL(test_vector_iir->get_delay(chan), delay);
        BOOST_CHECK_EQUAL(check_poke_delay(chan), delay);
        BOOST_CHECK_EQUAL(
            check_poke_beta(chan), calculate_alphabeta_register_value(beta));
        BOOST_CHECK_EQUAL(
            check_poke_alpha(chan), calculate_alphabeta_register_value(alpha));

        // Read max delay via API and make sure it agrees with the register
        BOOST_CHECK_EQUAL(
            test_vector_iir->get_max_delay(chan), check_peek_max_delay(chan));
    }
}

/*
 * This test case exercises the range checking performed on the vector IIR
 * settings, ensuring that the appropriate exception is thrown when out of
 * range.
 */
BOOST_FIXTURE_TEST_CASE(vector_iir_test_ranges, vector_iir_block_fixture)
{
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        BOOST_CHECK_THROW(test_vector_iir->set_alpha(-1.0, chan), uhd::value_error);
        BOOST_CHECK_THROW(test_vector_iir->set_alpha(15.0, chan), uhd::value_error);

        BOOST_CHECK_THROW(test_vector_iir->set_beta(-6.0, chan), uhd::value_error);
        BOOST_CHECK_THROW(test_vector_iir->set_beta(7.25, chan), uhd::value_error);

        BOOST_CHECK_THROW(test_vector_iir->set_delay(0, chan), uhd::value_error);
        BOOST_CHECK_THROW(test_vector_iir->set_delay(12345, chan), uhd::value_error);
        BOOST_CHECK_THROW(
            test_vector_iir->set_delay(test_vector_iir->get_max_delay(chan) + 1, chan),
            uhd::value_error);
    }
}
