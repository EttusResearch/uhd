//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fft_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t, adding a register poke override
 * that monitors the reset strobe address and sets a flag when written.
 */
class fft_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    void _poke_cb(uint32_t addr,
        uint32_t /*data*/,
        uhd::time_spec_t /*time*/,
        bool /*ack*/) override
    {
        if (addr == fft_block_control::REG_RESET_ADDR) {
            fft_was_reset = true;
        }
    }

    void reset_strobe()
    {
        fft_was_reset = false;
    }

    bool fft_was_reset = false;
};

/* fft_block_fixture is a class which is instantiated before each test case
 * is run. It sets up the block container, mock register interface, and
 * fft_block_control object, all of which are accessible to the test case.
 * The instance of the object is destroyed at the end of each test case.
 */

namespace {
constexpr size_t DEFAULT_MTU = 8000;
};

struct fft_block_fixture
{
    //! Create an FFT block and all related infrastructure for unit testsing.
    fft_block_fixture()
        : reg_iface(std::make_shared<fft_mock_reg_iface_t>())
        , block_container(get_mock_block(
              FFT_BLOCK, 1, 1, uhd::device_addr_t(), DEFAULT_MTU, ANY_DEVICE, reg_iface))
        , test_fft(block_container.get_block<fft_block_control>())
    {
        node_accessor.init_props(test_fft.get());
    }

    std::shared_ptr<fft_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<fft_block_control> test_fft;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the FFT block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(fft_test_construction, fft_block_fixture)
{
    // These are the defaults from the FFT block controller. They are not
    // exported, so duplicate them here. Obviously, if these defaults are
    // changed in fft_block_control, those changes must be reflected here.
    constexpr int fft_default_size_log2           = 8; // log2(256)
    constexpr fft_shift fft_default_shift         = fft_shift::NORMAL;
    constexpr fft_direction fft_default_direction = fft_direction::FORWARD;
    constexpr fft_magnitude fft_default_magnitude = fft_magnitude::COMPLEX;
    constexpr int fft_default_scaling             = 1706;

    BOOST_CHECK(reg_iface->fft_was_reset);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR],
        fft_default_size_log2);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_OUT_ADDR],
        static_cast<uint32_t>(fft_default_magnitude));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_DIRECTION_ADDR],
        static_cast<uint32_t>(fft_default_direction));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR],
        fft_default_scaling);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_SHIFT_CONFIG_ADDR],
        static_cast<uint32_t>(fft_default_shift));
}

/*
 * This test case exercises the remainder of the FFT block API with valid,
 * in-range values and ensures that the appropriate register is programmed
 * appropriately.
 */
BOOST_FIXTURE_TEST_CASE(fft_test_api, fft_block_fixture)
{
    constexpr fft_direction direction = fft_direction::REVERSE;
    test_fft->set_direction(direction);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_DIRECTION_ADDR],
        static_cast<uint32_t>(direction));
    BOOST_CHECK(test_fft->get_direction() == direction);

    constexpr fft_magnitude magnitude = fft_magnitude::MAGNITUDE_SQUARED;
    test_fft->set_magnitude(magnitude);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_OUT_ADDR],
        static_cast<uint32_t>(magnitude));
    BOOST_CHECK(test_fft->get_magnitude() == magnitude);

    constexpr fft_shift shift = fft_shift::NATURAL;
    test_fft->set_shift_config(shift);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_SHIFT_CONFIG_ADDR],
        static_cast<uint32_t>(shift));
    BOOST_CHECK(test_fft->get_shift_config() == shift);

    constexpr uint16_t scaling = 256;
    test_fft->set_scaling(scaling);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], scaling);
    BOOST_CHECK(test_fft->get_scaling() == scaling);

    constexpr size_t length_log2 = 10;
    test_fft->set_length(1 << length_log2);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR], length_log2);
    BOOST_CHECK_EQUAL(test_fft->get_length(), (1 << length_log2));

    // Make sure that the FFT length parameter is coerced to the closest
    // power of two smaller than the desired value.
    constexpr size_t coerced_length_log2 = 9;
    test_fft->set_length((1 << coerced_length_log2) + (1 << (coerced_length_log2 - 1)));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR],
        coerced_length_log2);
    BOOST_CHECK_EQUAL(test_fft->get_length(), (1 << coerced_length_log2));
}

/*
 * This test case exercises the range checking performed on several of the
 * FFT block properties, ensuring that the appropriate exception is thrown.
 */
BOOST_FIXTURE_TEST_CASE(fft_test_range_errors, fft_block_fixture)
{
    BOOST_CHECK_THROW(test_fft->set_property<int>("direction", 12345), uhd::value_error);
    BOOST_CHECK_THROW(test_fft->set_property<int>("magnitude", 54321), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fft->set_property<int>("shift_config", 31337), uhd::value_error);
    BOOST_CHECK_THROW(test_fft->set_scaling(32767), uhd::value_error);
    BOOST_CHECK_THROW(test_fft->set_length(65535), uhd::value_error);
}

/*
 * This test case ensures that the FFT block controller can be added
 * to a graph.
 */
BOOST_FIXTURE_TEST_CASE(fft_test_graph, fft_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info{
        0, 0, detail::graph_t::graph_edge_t::DYNAMIC, true};

    mock_radio_node_t mock_radio_block{0};
    mock_ddc_node_t mock_ddc_block{};
    mock_terminator_t mock_sink_term(2, {}, "MOCK_SINK");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    node_accessor.init_props(&mock_ddc_block);
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<size_t>(
        PROP_KEY_ATOMIC_ITEM_SIZE, 1234, {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_radio_block, &mock_ddc_block, edge_info);
    graph.connect(&mock_ddc_block, test_fft.get(), edge_info);
    graph.connect(test_fft.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    UHD_LOG_INFO("TEST", "Testing atomic item size manipulation...");
    // Try setting the atomic item size to some other value, it should bounce
    // back
    mock_sink_term.set_edge_property<size_t>(
        PROP_KEY_ATOMIC_ITEM_SIZE, 1996, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_fft->get_length() * 4,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
}
