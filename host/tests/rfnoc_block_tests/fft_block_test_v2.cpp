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
    fft_mock_reg_iface_t()
    {
        // Minor: 0 -> 0x0000
        // Major: 1 -> 0x0003
        read_memory[fft_block_control::REG_COMPAT_ADDR] = 0x00030000;
        // Max. FFT size: 16384 -> 0x0E
        // Max. CP length: 16383 -> 0x0E
        // Max. CP removal list length: 31 -> 0x05
        // Max. CP insertion list length: 31 -> 0x05
        read_memory[fft_block_control::REG_CAPABILITIES_ADDR] = 0x05050E0E;
        // magnitude squared supported: true -> 0b1000
        // magnitude supported: true -> 0b0100
        // FFT shift supported: true -> 0b0010
        // FFT bypass supported: true -> 0b0001
        read_memory[fft_block_control::REG_CAPABILITIES2_ADDR] = 0b1111;
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        if (addr >= fft_block_control::REG_RESET_ADDR_V1) {
            throw uhd::assertion_error(
                str(boost::format("Trying to write register %08x which is only supported "
                                  "for FFT Block v1")
                    % addr));
        } else if ((addr == fft_block_control::REG_COMPAT_ADDR)
                   || (addr == fft_block_control::REG_CAPABILITIES_ADDR)
                   || (addr == fft_block_control::REG_CAPABILITIES2_ADDR)
                   || (addr == fft_block_control::REG_CP_INS_LIST_OCC_ADDR)
                   || (addr == fft_block_control::REG_CP_REM_LIST_OCC_ADDR)) {
            throw uhd::assertion_error(
                str(boost::format("Trying to write to read-only register %08x") % addr));
        } else if (addr == fft_block_control::REG_CP_INS_LIST_LOAD_ADDR) {
            uint32_t cp_length = write_memory[fft_block_control::REG_CP_INS_LEN_ADDR];
            cp_insertion_list.push_back(cp_length);
        } else if (addr == fft_block_control::REG_CP_REM_LIST_LOAD_ADDR) {
            uint32_t cp_length = write_memory[fft_block_control::REG_CP_REM_LEN_ADDR];
            cp_removal_list.push_back(cp_length);
        } else if (addr == fft_block_control::REG_CP_INS_LIST_CLR_ADDR) {
            cp_insertion_list.clear();
        } else if (addr == fft_block_control::REG_CP_REM_LIST_CLR_ADDR) {
            cp_removal_list.clear();
        } else {
            write_memory[addr] = data;
        }
        UHD_LOG_TRACE("TEST", str(boost::format("poke [%04x] = %08x") % addr % data));
        if (addr == fft_block_control::REG_RESET_ADDR) {
            reset();
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        if (addr >= fft_block_control::REG_RESET_ADDR_V1) {
            throw uhd::assertion_error(
                str(boost::format("Trying to read register %08x which is only supported "
                                  "for FFT Block v1")
                    % addr));
        }
        if ((fft_block_control::REG_COMPAT_ADDR == addr)
            || (fft_block_control::REG_CAPABILITIES_ADDR == addr)
            || (fft_block_control::REG_CAPABILITIES2_ADDR == addr)) {
            // the value in read_memory was set during initialization
        } else if (fft_block_control::REG_CP_INS_LIST_OCC_ADDR == addr) {
            read_memory[addr] = cp_insertion_list.size();
        } else if (fft_block_control::REG_CP_REM_LIST_OCC_ADDR == addr) {
            read_memory[addr] = cp_removal_list.size();
        } else {
            read_memory[addr] = write_memory[addr];
        }
        UHD_LOG_TRACE(
            "TEST", str(boost::format("peek [%04x] = %08x") % addr % read_memory[addr]));
    }
    void reset()
    {
        fft_was_reset = true;
        cp_insertion_list.clear();
        cp_removal_list.clear();
    }

    bool fft_was_reset = false;
    std::vector<uint32_t> cp_insertion_list;
    std::vector<uint32_t> cp_removal_list;
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
        , block_container(get_mock_block(FFT_BLOCK_V2,
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
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_LENGTH_LOG2_ADDR],
        fft_default_size_log2);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_ADDR],
        static_cast<uint32_t>(fft_default_magnitude));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_DIRECTION_ADDR],
        static_cast<uint32_t>(fft_default_direction));
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR],
        fft_default_scaling);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_ORDER_ADDR],
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
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_MAGNITUDE_ADDR],
        static_cast<uint32_t>(magnitude));
    BOOST_CHECK(test_fft->get_magnitude() == magnitude);

    constexpr fft_shift shift = fft_shift::NATURAL;
    test_fft->set_shift_config(shift);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_ORDER_ADDR],
        static_cast<uint32_t>(shift));
    BOOST_CHECK(test_fft->get_shift_config() == shift);

    constexpr uint16_t scaling = 0xFF;
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
    UHD_LOG_INFO("TEST", "Testing atomic item size manipulation (allowed values)");
    // Try setting the atomic item size to some allowed values (FFT length,
    // half the FFT length)
    mock_sink_term.set_edge_property<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
        test_fft->get_length() * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_fft->get_length() * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
    mock_sink_term.set_edge_property<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
        (test_fft->get_length() / 2) * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL((test_fft->get_length() / 2) * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));

    UHD_LOG_INFO(
        "TEST", "Testing atomic item size manipulation (limiting to FFT length)");
    // Try setting the atomic item size to some a value bigger than the FFT
    // length, it should be limited to the FFT length
    mock_sink_term.set_edge_property<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
        (test_fft->get_length() * 2) * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_fft->get_length() * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));

    UHD_LOG_INFO(
        "TEST", "Testing atomic item size manipulation (CP insertion list filled)");
    // Add elements to the CP insertion FIFO -> Atomic item size should switch
    // to 1 * BYTES_PER_SAMPLE
    test_fft->set_cp_insertion_list({140});
    BOOST_CHECK_EQUAL(BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
    test_fft->set_length(512);
    BOOST_CHECK_EQUAL(BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));

    UHD_LOG_INFO(
        "TEST", "Testing atomic item size manipulation (CP insertion list empty again)");
    // CP insertion list is empty again -> Allowed values from test above
    // should work again
    test_fft->set_cp_insertion_list({});
    mock_sink_term.set_edge_property<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
        test_fft->get_length() * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_fft->get_length() * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
    mock_sink_term.set_edge_property<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
        (test_fft->get_length() / 2) * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL((test_fft->get_length() / 2) * BYTES_PER_SAMPLE,
        mock_sink_term.get_edge_property<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, {res_source_info::INPUT_EDGE, 0}));
}

/*
 * This test case ensures that features from FFT block v2 are reported as not
 * supported
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_capabilities, fft_block_fixture)
{
    BOOST_CHECK_NE(test_fft->get_max_cp_length(), 0);
    BOOST_CHECK_NE(test_fft->get_max_cp_insertion_list_length(), 0);
    BOOST_CHECK_NE(test_fft->get_max_cp_removal_list_length(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_cp_insertion_list().size(), 0);
    BOOST_CHECK_EQUAL(test_fft->get_cp_removal_list().size(), 0);
}

/*
 * This test case ensures that valid CP removal / insertion lists can be set
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_valid_lists, fft_block_fixture)
{
    std::vector<uint32_t> cp_lengths_requested, cp_lengths_read;
    uint32_t max_cp_length      = test_fft->get_max_cp_length();
    uint32_t max_cp_list_length = test_fft->get_max_cp_insertion_list_length();

    for (int testcase = 0; testcase <= 3; testcase++) {
        if (testcase == 0) {
            UHD_LOG_INFO("TEST", "Empty list (Clear the FIFO)");
            cp_lengths_requested.clear();
        } else if (testcase == 1) {
            UHD_LOG_INFO("TEST", "one element of max size");
            cp_lengths_requested = {max_cp_length};
        } else if (testcase == 2) {
            UHD_LOG_INFO("TEST", "Multiple elements, up to max. size");
            cp_lengths_requested = {0, 1, 2, max_cp_length - 1, max_cp_length};
        } else if (testcase == 3) {
            UHD_LOG_INFO("TEST", "Max. possible elements, each max. size");
            cp_lengths_requested.clear();
            for (uint32_t i = 0; i < max_cp_list_length; i++) {
                cp_lengths_requested.push_back(max_cp_length);
            }
        }

        // ... load CP insertion FIFO
        test_fft->set_cp_insertion_list(cp_lengths_requested);
        cp_lengths_read = test_fft->get_cp_insertion_list();
        BOOST_CHECK_EQUAL(cp_lengths_requested.size(), cp_lengths_read.size());
        for (uint32_t i = 0; i < cp_lengths_requested.size(); i++) {
            BOOST_CHECK_EQUAL(cp_lengths_requested[i], cp_lengths_read[i]);
        }
        // ... load CP removal FIFO
        test_fft->set_cp_removal_list(cp_lengths_requested);
        cp_lengths_read = test_fft->get_cp_removal_list();
        BOOST_CHECK_EQUAL(cp_lengths_requested.size(), cp_lengths_read.size());
        for (uint32_t i = 0; i < cp_lengths_requested.size(); i++) {
            BOOST_CHECK_EQUAL(cp_lengths_requested[i], cp_lengths_read[i]);
        }
    }
}

/*
 * This test case ensures that non-empty CP removal / insertion lists are rejected
 */
BOOST_FIXTURE_TEST_CASE(fft_test_cp_invalid_lists, fft_block_fixture)
{
    std::vector<uint32_t> cp_lengths_requested;
    uint32_t max_cp_length      = test_fft->get_max_cp_length();
    uint32_t max_cp_list_length = test_fft->get_max_cp_insertion_list_length();


    for (int testcase = 0; testcase <= 2; testcase++) {
        if (testcase == 0) {
            UHD_LOG_INFO("TEST", "One Element; too big");
            cp_lengths_requested = {max_cp_length + 1};
        } else if (testcase == 1) {
            UHD_LOG_INFO("TEST", "Multiple elements, one too big");
            cp_lengths_requested = {0, 1, 2, max_cp_length + 1};
        } else if (testcase == 2) {
            UHD_LOG_INFO("TEST", "List length too long");
            cp_lengths_requested.clear();
            for (uint32_t i = 0; i <= max_cp_list_length; i++) {
                cp_lengths_requested.push_back(10);
            }
        }

        // ... load CP insertion FIFO
        BOOST_CHECK_THROW(
            test_fft->set_cp_insertion_list(cp_lengths_requested), uhd::value_error);
        // ... load CP removal FIFO
        BOOST_CHECK_THROW(
            test_fft->set_cp_removal_list(cp_lengths_requested), uhd::value_error);
    }
}

/*
 * This test case checks the max. supported values
 */
BOOST_FIXTURE_TEST_CASE(fft_test_supported_values, fft_block_fixture)
{
    // as function calls
    BOOST_CHECK_EQUAL(test_fft->get_max_length(), 16384);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_length(), 16383);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_insertion_list_length(), 31);
    BOOST_CHECK_EQUAL(test_fft->get_max_cp_removal_list_length(), 31);
    // as properties
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_length"), 16384);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_length"), 16383);
    BOOST_CHECK_EQUAL(
        test_fft->get_property<uint32_t>("max_cp_insertion_list_length"), 31);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_removal_list_length"), 31);
    // check that the properties are RO
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_length", 1024),
    // uhd::access_error);
    test_fft->set_property<uint32_t>("max_length", 1024);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_length"), 16384);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_length", 10),
    // uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_length", 10);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_length"), 16383);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_insertion_list_length",
    // 10), uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_insertion_list_length", 10);
    BOOST_CHECK_EQUAL(
        test_fft->get_property<uint32_t>("max_cp_insertion_list_length"), 31);
    // BOOST_CHECK_THROW(test_fft->set_property<uint32_t>("max_cp_removal_list_length",
    // 10), uhd::access_error);
    test_fft->set_property<uint32_t>("max_cp_removal_list_length", 10);
    BOOST_CHECK_EQUAL(test_fft->get_property<uint32_t>("max_cp_removal_list_length"), 31);
}

/*
 * This test case checks the set_scaling_factor function
 */
BOOST_FIXTURE_TEST_CASE(fft_test_set_scaling_factor, fft_block_fixture)
{
    test_fft->set_length(256);
    test_fft->set_scaling_factor(1.0 / 1);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000000000);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000000);
    test_fft->set_scaling_factor(1.0 / 2);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000000001);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000001);
    test_fft->set_scaling_factor(1.0 / 4);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000000010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000010);
    test_fft->set_scaling_factor(1.0 / 8);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000000110);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000000110);
    test_fft->set_scaling_factor(1.0 / 16);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000001010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000001010);
    test_fft->set_scaling_factor(1.0 / 32);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000011010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000011010);
    test_fft->set_scaling_factor(1.0 / 64);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000000101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000000101010);
    BOOST_CHECK_THROW(test_fft->set_scaling_factor(1.0 / 512), uhd::value_error);
    test_fft->set_length(4096);
    test_fft->set_scaling_factor(1.0 / 512);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b000110101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b000110101010);
    test_fft->set_scaling_factor(1.0 / 1024);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b001010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b001010101010);
    test_fft->set_scaling_factor(1.0 / 2048);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b011010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b011010101010);
    test_fft->set_scaling_factor(1.0 / 4096);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[fft_block_control::REG_SCALING_ADDR], 0b101010101010);
    BOOST_CHECK_EQUAL(test_fft->get_scaling(), 0b101010101010);
}

/*
 * This test case checks the set_bypass_mode function
 */
BOOST_FIXTURE_TEST_CASE(fft_test_set_bypass_mode, fft_block_fixture)
{
    test_fft->set_bypass_mode(true);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_BYPASS_ADDR], 0b1);
    BOOST_CHECK_EQUAL(test_fft->get_bypass_mode(), true);
    test_fft->set_bypass_mode(false);
    BOOST_CHECK_EQUAL(reg_iface->write_memory[fft_block_control::REG_BYPASS_ADDR], 0b0);
    BOOST_CHECK_EQUAL(test_fft->get_bypass_mode(), false);
}
