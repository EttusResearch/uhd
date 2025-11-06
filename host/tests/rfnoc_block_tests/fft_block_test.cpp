//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/detail/graph.hpp>
#include <uhd/rfnoc/fft_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/mock_nodes.hpp>
#include <uhd/rfnoc/node_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;
using namespace uhd::rfnoc::test;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t, adding a register poke override
 * that monitors the reset strobe address and sets a flag when written.
 */
class fft_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        const auto UHD_UNUSED(_data) = data;
        if ((addr >= fft_block_control::REG_COMPAT_ADDR)
            && (addr <= fft_block_control::REG_CP_REM_LIST_OCC_ADDR)) {
            throw uhd::assertion_error(
                str(boost::format("Trying to read register %08x which is only supported "
                                  "for FFT Block v2")
                    % addr));
        }
        UHD_LOG_TRACE("TEST", str(boost::format("poke [%04x] = %08x") % addr % data));
        if (addr == fft_block_control::REG_RESET_ADDR_V1) {
            fft_was_reset = true;
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        if ((addr >= fft_block_control::REG_COMPAT_ADDR)
            && (addr <= fft_block_control::REG_CP_REM_LIST_OCC_ADDR)) {
            throw uhd::assertion_error(
                str(boost::format("Trying to read register %08x which is only supported "
                                  "for FFT Block v2")
                    % addr));
        }
        read_memory[addr] = write_memory[addr];
        UHD_LOG_TRACE(
            "TEST", str(boost::format("peek [%04x] = %08x") % addr % read_memory[addr]));
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
        , block_container(get_mock_block(FFT_BLOCK_V1,
              1,
              1,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
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
    constexpr int fft_default_scaling             = 0b10101010;

    BOOST_CHECK(reg_iface->fft_was_reset);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR_V1],
        fft_default_size_log2);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_ADDR_V1],
        static_cast<uint32_t>(fft_default_magnitude));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_DIRECTION_ADDR_V1],
        static_cast<uint32_t>(fft_default_direction));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1],
        fft_default_scaling);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_ORDER_ADDR_V1],
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
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_DIRECTION_ADDR_V1],
        static_cast<uint32_t>(direction));
    BOOST_CHECK(test_fft->get_direction() == direction);

    constexpr fft_magnitude magnitude = fft_magnitude::MAGNITUDE_SQUARED;
    test_fft->set_magnitude(magnitude);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_ADDR_V1],
        static_cast<uint32_t>(magnitude));
    BOOST_CHECK(test_fft->get_magnitude() == magnitude);

    constexpr fft_shift shift = fft_shift::NATURAL;
    test_fft->set_shift_config(shift);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_ORDER_ADDR_V1],
        static_cast<uint32_t>(shift));
    BOOST_CHECK(test_fft->get_shift_config() == shift);

    constexpr uint16_t scaling = 0xFF;
    test_fft->set_scaling(scaling);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], scaling);
    BOOST_CHECK(test_fft->get_scaling() == scaling);

    constexpr size_t length_log2 = 10;
    test_fft->set_length(1 << length_log2);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR_V1], length_log2);
    BOOST_CHECK_EQUAL(test_fft->get_length(), (1 << length_log2));

    // Make sure that the FFT length parameter is coerced to the closest
    // power of two smaller than the desired value.
    constexpr size_t coerced_length_log2 = 9;
    test_fft->set_length((1 << coerced_length_log2) + (1 << (coerced_length_log2 - 1)));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR_V1],
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
    BOOST_CHECK_THROW(test_fft->set_length(4), uhd::value_error);
    BOOST_CHECK_THROW(test_fft->set_length(7), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fft->set_length(test_fft->get_max_length() + 1), uhd::value_error);
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

    constexpr size_t BYTES_PER_SAMPLE = 4;
    UHD_LOG_INFO("TEST", "Testing atomic item size manipulation...");
    // Try setting the atomic item size to some other value, it should bounce
    // back
    mock_sink_term.set_edge_property<size_t>(
        PROP_KEY_ATOMIC_ITEM_SIZE, 1996, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_fft->get_length() * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
    // Set another fft_length, the atomic item size should be updated
    test_fft->set_length(512);
    BOOST_CHECK_EQUAL(test_fft->get_length() * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
}

/*
 * This test case ensures that CP removal / insertion capabilities are zero
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_capabilities, fft_block_fixture)
{
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_insertion_list_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_removal_list_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_cp_insertion_list().size(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_cp_removal_list().size(), 0);
}

/*
 * This test case ensures that valid (empty) CP removal / insertion lists can be
 * set
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_valid_lists, fft_block_fixture)
{
    test_fft->set_cp_insertion_list(std::vector<uint32_t>());
    test_fft->set_cp_removal_list(std::vector<uint32_t>());
    BOOST_CHECK_EQUAL(test_fft->get_cp_insertion_list().size(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_cp_removal_list().size(), 0);
}

/*
 * This test case ensures that invalid (non-empty) CP removal / insertion lists
 * are rejected
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_invalid_lists, fft_block_fixture)
{
    std::vector<uint32_t> cp_lengths = {0, 1, 2, 3};
    BOOST_CHECK_THROW(test_fft->set_cp_insertion_list(cp_lengths), uhd::value_error);
    BOOST_CHECK_THROW(test_fft->set_cp_removal_list(cp_lengths), uhd::value_error);
}

/*
 * This test case checks the max. supported values
 */
BOOST_FIXTURE_TEST_CASE(fft_test_supported_values, fft_block_fixture)
{
    // as function calls
    BOOST_CHECK_EQUAL(test_fft->get_max_length(), 4096);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_insertion_list_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_removal_list_length(), 0);
    // as properties
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_length"), 4096);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_length"), 0);
    BOOST_CHECK_EQUAL(
        test_fft->get_property<uint32_t>("max_cp_insertion_list_length"), 0);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_removal_list_length"), 0);
    // check that the properties are RO
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_length", 1024),
    // uhd::access_error);
    test_fft->set_property<uint32_t>("max_length", 1024);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_length"), 4096);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_length", 10),
    // uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_length", 10);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_length"), 0);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_insertion_list_length",
    // 10), uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_insertion_list_length", 10);
    BOOST_CHECK_EQUAL(
        test_fft->get_property<uint32_t>("max_cp_insertion_list_length"), 0);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_removal_list_length",
    // 10), uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_removal_list_length", 10);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_removal_list_length"), 0);
}

/*
 * This test case checks the set_scaling_factor function
 */
BOOST_FIXTURE_TEST_CASE(fft_test_set_scaling_factor, fft_block_fixture)
{
    test_fft->set_length(256);
    test_fft->set_scaling_factor(1.0 / 1);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000000000);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000000);
    test_fft->set_scaling_factor(1.0 / 2);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000000001);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000001);
    test_fft->set_scaling_factor(1.0 / 4);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000000010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000010);
    test_fft->set_scaling_factor(1.0 / 8);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000000110);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000110);
    test_fft->set_scaling_factor(1.0 / 16);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000001010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000001010);
    test_fft->set_scaling_factor(1.0 / 32);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000011010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000011010);
    test_fft->set_scaling_factor(1.0 / 64);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000000101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000101010);
    BOOST_CHECK_THROW(test_fft->set_scaling_factor(1.0 / 512), uhd::value_error);
    test_fft->set_length(4096);
    test_fft->set_scaling_factor(1.0 / 512);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b000110101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000110101010);
    test_fft->set_scaling_factor(1.0 / 1024);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b001010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b001010101010);
    test_fft->set_scaling_factor(1.0 / 2048);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b011010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b011010101010);
    test_fft->set_scaling_factor(1.0 / 4096);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR_V1], 0b101010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b101010101010);
}

/*
 * This test case checks the set_bypass_mode function
 */
BOOST_FIXTURE_TEST_CASE(fft_test_set_bypass_mode, fft_block_fixture)
{
    BOOST_CHECK_THROW(test_fft->set_bypass_mode(true), uhd::value_error);
    test_fft->set_bypass_mode(false);
    BOOST_CHECK_EQUAL(test_fft->get_bypass_mode(), false);
}
