//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/fir_filter_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t by adding poke and peek hooks that
 * monitor writes and reads to the registers implemented within the FIR
 * filter RFNoC block hardware and emulating the expected behavior of the
 * hardware when those registers are read and written. For instance, writes
 * to the coefficient registers store the coefficients in a vector and
 * track the position of the last write to the REG_FIR_LOAD_COEFF_LAST_ADDR
 * register to allow the unit test to gauge the proper operation of the FIR
 * filter block controller.
 */
class fir_filter_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    fir_filter_mock_reg_iface_t(size_t num_chans, std::vector<size_t> max_num_coeffs)
        : last_coeff_write_pos(num_chans, 0)
        , coeffs(num_chans)
        , _num_chans(num_chans)
        , _max_num_coeffs(max_num_coeffs)
    {
        reset();
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        size_t chan   = addr / fir_filter_block_control::REG_FIR_BLOCK_SIZE;
        size_t offset = addr % fir_filter_block_control::REG_FIR_BLOCK_SIZE;
        if (chan >= _num_chans) {
            throw uhd::assertion_error("Invalid channel index");
        }

        if (offset == fir_filter_block_control::REG_FIR_MAX_NUM_COEFFS_ADDR) {
            throw uhd::assertion_error("Invalid write to read-only register");
        } else if (offset == fir_filter_block_control::REG_FIR_LOAD_COEFF_ADDR) {
            coeffs.at(chan).push_back(uhd::narrow_cast<int16_t>(data));
        } else if (offset == fir_filter_block_control::REG_FIR_LOAD_COEFF_LAST_ADDR) {
            last_coeff_write_pos[chan] = coeffs.at(chan).size();
            coeffs.at(chan).push_back(uhd::narrow_cast<int16_t>(data));
        } else {
            throw uhd::assertion_error("Invalid write to out of bounds address");
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        size_t chan   = addr / fir_filter_block_control::REG_FIR_BLOCK_SIZE;
        size_t offset = addr % fir_filter_block_control::REG_FIR_BLOCK_SIZE;
        if (chan >= _num_chans) {
            throw uhd::assertion_error("Invalid channel index");
        }

        if (offset == fir_filter_block_control::REG_FIR_MAX_NUM_COEFFS_ADDR) {
            read_memory[addr] = uhd::narrow_cast<int32_t>(_max_num_coeffs.at(chan));
        } else {
            throw uhd::assertion_error("Invalid read from out of bounds address");
        }
    }

    void reset()
    {
        for (size_t chan = 0; chan < _num_chans; chan++) {
            last_coeff_write_pos[chan] = 0;
            coeffs.at(chan).clear();
        }
    }

    std::vector<size_t> last_coeff_write_pos;
    std::vector<std::vector<int16_t>> coeffs;

private:
    const size_t _num_chans;
    const std::vector<size_t> _max_num_coeffs;
};


/* fir_filter_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and fir_filter_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */
constexpr size_t DEFAULT_MTU = 8000;
constexpr size_t NUM_CHANS   = 4;
static const std::vector<size_t> MAX_NUM_COEFFS{1, 2, 1337, 65537};

struct fir_filter_block_fixture
{
    fir_filter_block_fixture()
        : reg_iface(std::make_shared<fir_filter_mock_reg_iface_t>(NUM_CHANS, MAX_NUM_COEFFS))
        , block_container(get_mock_block(FIR_FILTER_BLOCK,
              NUM_CHANS,
              NUM_CHANS,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_fir_filter(block_container.get_block<fir_filter_block_control>())
    {
        node_accessor.init_props(test_fir_filter.get());
    }

    std::shared_ptr<fir_filter_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<fir_filter_block_control> test_fir_filter;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the fir_filter block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(fir_filter_test_construction, fir_filter_block_fixture)
{
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        // Check that the number of coefficients is expected
        BOOST_CHECK_EQUAL(reg_iface->coeffs.at(chan).size(), MAX_NUM_COEFFS.at(chan));
        // Check that the first coefficient is the maximum positive int16_t
        // value and then all other values are 0 (i.e. impulse)
        BOOST_CHECK_EQUAL(
            reg_iface->coeffs.at(chan).at(0), std::numeric_limits<int16_t>::max());
        for (size_t i = 1; i < reg_iface->coeffs.at(chan).size(); i++) {
            BOOST_CHECK_EQUAL(reg_iface->coeffs.at(chan).at(i), 0);
        }
        // Check that the LOAD_COEFF_LAST register was written at the right
        // time (i.e. with the last value)
        BOOST_CHECK_EQUAL(
            reg_iface->last_coeff_write_pos.at(chan), MAX_NUM_COEFFS.at(chan) - 1);
    }
}

/*
 * This test case exercises the get_max_num_coefficients() API.
 */
BOOST_FIXTURE_TEST_CASE(fir_filter_test_max_num_coeffs, fir_filter_block_fixture)
{
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        BOOST_CHECK_EQUAL(test_fir_filter->get_max_num_coefficients(chan), MAX_NUM_COEFFS.at(chan));
    }
}

/*
 * This test case exercises the set_coefficients() API and get_coefficients()
 * APIs and ensures that the hardware registers are programmed appropriately
 * when new coefficients are specified.
 */
BOOST_FIXTURE_TEST_CASE(fir_filter_test_set_get_coefficients, fir_filter_block_fixture)
{
    // Reset state of mock FIR filter register interface
    reg_iface->reset();

    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        // Generate some dummy coefficients
        const size_t num_coeffs = test_fir_filter->get_max_num_coefficients(chan);
        const std::vector<int16_t> coeffs(num_coeffs, chan);
        test_fir_filter->set_coefficients(coeffs, chan);

        // Check that all coefficients were written
        BOOST_CHECK_EQUAL(reg_iface->coeffs.at(chan).size(), num_coeffs);

        // Check correctness of coefficients
        for (size_t i = 0; i < coeffs.size(); i++) {
            BOOST_CHECK_EQUAL(reg_iface->coeffs.at(chan).at(i), chan);
        }
        // Check that the LOAD_COEFF_LAST register was written at the right
        // time (i.e. with the last value)
        BOOST_CHECK_EQUAL(reg_iface->last_coeff_write_pos.at(chan), num_coeffs - 1);

        // Verify that get_coefficients() returns what we expect
        const std::vector<int16_t> received_coeffs = test_fir_filter->get_coefficients(chan);

        BOOST_CHECK_EQUAL(received_coeffs.size(), num_coeffs);

        // Check correctness of returned coefficients
        for (size_t i = 0; i < coeffs.size(); i++) {
            BOOST_CHECK_EQUAL(received_coeffs.at(i), coeffs.at(i));
        }

        // Now send only one coefficent and ensure the rest are zero padded
        const std::vector<int16_t> coeffs2{std::numeric_limits<int16_t>::max()};

        // Reset recorded coefficients in mock reg iface
        reg_iface->reset();

        test_fir_filter->set_coefficients(coeffs2, chan);

        // Verify that get_coefficients() returns what we expect
        const std::vector<int16_t> received_coeffs2 = test_fir_filter->get_coefficients(chan);
        BOOST_CHECK_EQUAL(received_coeffs2.at(0), coeffs2.at(0));
        for (size_t i = 1; i < num_coeffs; i++) {
            BOOST_CHECK_EQUAL(received_coeffs2.at(i), 0);
        }
        // Check that the LOAD_COEFF_LAST register was written at the right time
        BOOST_CHECK_EQUAL(reg_iface->last_coeff_write_pos.at(chan), num_coeffs - 1);
    }
}

/*
 * This test case exercises the coefficient length checking of
 * set_coefficients().
 */
BOOST_FIXTURE_TEST_CASE(fir_filter_test_length_error, fir_filter_block_fixture)
{
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        const size_t num_coeffs = test_fir_filter->get_max_num_coefficients(chan);
        const std::vector<int16_t> coeffs(num_coeffs * 2);
        BOOST_CHECK_THROW(test_fir_filter->set_coefficients(coeffs, chan), uhd::value_error);
    }
}

/*
 * This test case ensures that the FIR filter block can be added to
 * an RFNoC graph.
 */
BOOST_FIXTURE_TEST_CASE(fir_filter_test_graph, fir_filter_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info;
    edge_port_info.src_port                    = 0;
    edge_port_info.dst_port                    = 0;
    edge_port_info.property_propagation_active = true;
    edge_port_info.edge                        = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_radio_node_t mock_radio_block{0};
    mock_ddc_node_t mock_ddc_block{};
    mock_terminator_t mock_sink_term(1, {}, "MOCK_SINK");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    node_accessor.init_props(&mock_ddc_block);
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 1});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_radio_block, &mock_ddc_block, edge_port_info);
    graph.connect(&mock_ddc_block, test_fir_filter.get(), edge_port_info);
    graph.connect(test_fir_filter.get(), &mock_sink_term, edge_port_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}
