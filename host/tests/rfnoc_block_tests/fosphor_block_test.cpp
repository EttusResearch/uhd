//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fosphor_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t, handling three particular registers
 * (REG_ENABLE, REG_RANDOM, and REG_WF_CTRL) specially by allowing values
 * written to those registers to be subsequently read back, which isn't
 * supported by mock_reg_iface_t as read and write register spaces are
 * segregated into different map objects. The fosphor_block_controller
 * modifies bitfields within these three registers while keeping the
 * remainder of the register contents intact, necessitating this capability
 * in its mock register interface.
 */
class fosphor_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        if (addr == fosphor_block_control::REG_ENABLE_ADDR) {
            fosphor_enable_reg = data;
        } else if (addr == fosphor_block_control::REG_CLEAR_ADDR) {
            fosphor_core_reset    = (fosphor_core_reset | (data & 0x02));
            fosphor_history_reset = (fosphor_core_reset | (data & 0x01));
        } else if (addr == fosphor_block_control::REG_RANDOM_ADDR) {
            fosphor_random_reg = data;
        } else if (addr == fosphor_block_control::REG_WF_CTRL_ADDR) {
            fosphor_wf_control_reg = data;
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        if (addr == fosphor_block_control::REG_ENABLE_ADDR) {
            read_memory[addr] = fosphor_enable_reg;
        } else if (addr == fosphor_block_control::REG_RANDOM_ADDR) {
            read_memory[addr] = fosphor_random_reg;
        } else if (addr == fosphor_block_control::REG_WF_CTRL_ADDR) {
            read_memory[addr] = fosphor_wf_control_reg;
        }
    }

    void reset_clear_strobes()
    {
        fosphor_core_reset    = false;
        fosphor_history_reset = false;
    }

    uint32_t fosphor_enable_reg     = 0;
    uint32_t fosphor_random_reg     = 0;
    uint32_t fosphor_wf_control_reg = 0;
    bool fosphor_core_reset         = false;
    bool fosphor_history_reset      = false;
};

/* fosphor_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and fosphor_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */

namespace {
constexpr size_t DEFAULT_MTU = 8000;
};

struct fosphor_block_fixture
{
    fosphor_block_fixture()
        : reg_iface(std::make_shared<fosphor_mock_reg_iface_t>())
        , block_container(get_mock_block(FOSPHOR_BLOCK,
              1,
              2,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_fosphor(block_container.get_block<fosphor_block_control>())
    {
        node_accessor.init_props(test_fosphor.get());
    }

    std::shared_ptr<fosphor_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<fosphor_block_control> test_fosphor;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the fosphor block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_construction, fosphor_block_fixture)
{
    // Check that both histogram and waveforms are enabled by default
    BOOST_CHECK_EQUAL(reg_iface->fosphor_enable_reg, 3);
    // Check that both dither and noise are enabled by default
    BOOST_CHECK_EQUAL(reg_iface->fosphor_random_reg, 3);
    // Check that the waterfall mode is max hold and the predivision ratio
    // is 1:1
    BOOST_CHECK_EQUAL(reg_iface->fosphor_wf_control_reg, 0);
    // Check that the core and history resets are strobed upon construction
    BOOST_CHECK(reg_iface->fosphor_core_reset);
    BOOST_CHECK(reg_iface->fosphor_history_reset);

    // Check that the remainder of the registers were written with the
    // expected initial values.
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_DECIM_ADDR), 0);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_OFFSET_ADDR), 0);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_SCALE_ADDR), 256);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_TRISE_ADDR), 4096);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_TDECAY_ADDR), 16384);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_ALPHA_ADDR), 65280);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_EPSILON_ADDR), 1);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_WF_DECIM_ADDR), 6);
}

/*
 * This test case exercises the histogram and waterfall enable APIs
 * and ensures that the enable register is programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_enables, fosphor_block_fixture)
{
    // Note: initial condition is histogram and waterfall enabled
    test_fosphor->set_enable_histogram(false);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_enable_reg, 2); // WF, no histo
    BOOST_CHECK(!test_fosphor->get_enable_histogram());

    test_fosphor->set_enable_waterfall(false);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_enable_reg, 0);
    BOOST_CHECK(!test_fosphor->get_enable_waterfall());

    test_fosphor->set_enable_histogram(true);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_enable_reg, 1); // histo, no WF
    BOOST_CHECK(test_fosphor->get_enable_histogram());

    test_fosphor->set_enable_waterfall(true);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_enable_reg, 3);
    BOOST_CHECK(test_fosphor->get_enable_waterfall());
}

/*
 * This test case exercises the randomness (dither and noise) enable APIs
 * and ensures that the random register is programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_randomness_enables, fosphor_block_fixture)
{
    // Note: initial condition is dither and noise enabled
    test_fosphor->set_enable_dither(false);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_random_reg, 2); // Noise, no dither
    BOOST_CHECK(!test_fosphor->get_enable_dither());

    test_fosphor->set_enable_noise(false);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_random_reg, 0);
    BOOST_CHECK(!test_fosphor->get_enable_noise());

    test_fosphor->set_enable_dither(true);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_random_reg, 1); // Dither, no noise
    BOOST_CHECK(test_fosphor->get_enable_dither());

    test_fosphor->set_enable_noise(true);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_random_reg, 3);
    BOOST_CHECK(test_fosphor->get_enable_noise());
}

/*
 * This test case exercises the waterfall control APIs and ensure that the
 * waterfall control register is programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_waterfall_control, fosphor_block_fixture)
{
    // Note: initial condition is max hold waterfall mode
    test_fosphor->set_waterfall_mode(fosphor_waterfall_mode::AVERAGE);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_wf_control_reg, 0x80); // bit 7 set
    BOOST_CHECK(test_fosphor->get_waterfall_mode() == fosphor_waterfall_mode::AVERAGE);

    std::vector<fosphor_waterfall_predivision_ratio> ratios{
        fosphor_waterfall_predivision_ratio::RATIO_1_8,
        fosphor_waterfall_predivision_ratio::RATIO_1_256,
        fosphor_waterfall_predivision_ratio::RATIO_1_64,
        fosphor_waterfall_predivision_ratio::RATIO_1_1};

    for (auto ratio : ratios) {
        test_fosphor->set_waterfall_predivision(ratio);
        // Bottom 2 bits correspond to the predivision ratios
        BOOST_CHECK_EQUAL(
            reg_iface->fosphor_wf_control_reg, 0x80 | static_cast<uint32_t>(ratio));
        BOOST_CHECK(test_fosphor->get_waterfall_predivision() == ratio);
    }

    test_fosphor->set_waterfall_mode(fosphor_waterfall_mode::MAX_HOLD);
    BOOST_CHECK_EQUAL(reg_iface->fosphor_wf_control_reg, 0); // bit 7 clear
    BOOST_CHECK(test_fosphor->get_waterfall_mode() == fosphor_waterfall_mode::MAX_HOLD);
}

/*
 * This test case exercises the remainder of the fosphor block API with
 * valid, in-range values and ensures that the appropriate register is
 * programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_api, fosphor_block_fixture)
{
    reg_iface->reset_clear_strobes();
    test_fosphor->clear_history();
    // Ensure that the history clear bit is strobed
    BOOST_CHECK(reg_iface->fosphor_history_reset);

    test_fosphor->set_histogram_decimation(150);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_DECIM_ADDR), 148);
    BOOST_CHECK_EQUAL(test_fosphor->get_histogram_decimation(), 150);

    test_fosphor->set_histogram_offset(2513);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_OFFSET_ADDR), 2513);
    BOOST_CHECK_EQUAL(test_fosphor->get_histogram_offset(), 2513);

    test_fosphor->set_histogram_scale(15827);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_SCALE_ADDR), 15827);
    BOOST_CHECK_EQUAL(test_fosphor->get_histogram_scale(), 15827);

    test_fosphor->set_histogram_rise_rate(8191);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_TRISE_ADDR), 8191);
    BOOST_CHECK_EQUAL(test_fosphor->get_histogram_rise_rate(), 8191);

    test_fosphor->set_histogram_decay_rate(53280);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_TDECAY_ADDR), 53280);
    BOOST_CHECK_EQUAL(test_fosphor->get_histogram_decay_rate(), 53280);

    test_fosphor->set_spectrum_alpha(500);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_ALPHA_ADDR), 500);
    BOOST_CHECK_EQUAL(test_fosphor->get_spectrum_alpha(), 500);

    test_fosphor->set_spectrum_max_hold_decay(55296);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_EPSILON_ADDR), 55296);
    BOOST_CHECK_EQUAL(test_fosphor->get_spectrum_max_hold_decay(), 55296);

    test_fosphor->set_waterfall_decimation(192);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory.at(fosphor_block_control::REG_WF_DECIM_ADDR), 190);
    BOOST_CHECK_EQUAL(test_fosphor->get_waterfall_decimation(), 192);
}

/*
 * This test case exercises the range checking performed on several of the
 * fosphor properties, ensuring that the appropriate exception is throw.
 * Note that in some cases, the property is set directly via the
 * set_property() call because the data type of the corresponding API call
 * narrows or restricts the values to an acceptable range for the property.
 */
BOOST_FIXTURE_TEST_CASE(fosphor_test_range_errors, fosphor_block_fixture)
{
    BOOST_CHECK_THROW(test_fosphor->set_histogram_decimation(0), uhd::value_error);
    BOOST_CHECK_THROW(test_fosphor->set_histogram_decimation(32768), uhd::value_error);

    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("offset", -256123), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("offset", 262144), uhd::value_error);

    BOOST_CHECK_THROW(test_fosphor->set_property<int>("scale", -512), uhd::value_error);
    BOOST_CHECK_THROW(test_fosphor->set_property<int>("scale", 100000), uhd::value_error);

    BOOST_CHECK_THROW(test_fosphor->set_property<int>("trise", -32767), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("trise", 1048576), uhd::value_error);

    BOOST_CHECK_THROW(test_fosphor->set_property<int>("tdecay", -16), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("tdecay", 1234567), uhd::value_error);

    BOOST_CHECK_THROW(test_fosphor->set_property<int>("alpha", -1), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("alpha", 1000000), uhd::value_error);

    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("epsilon", -123456), uhd::value_error);
    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("epsilon", 524288), uhd::value_error);

    BOOST_CHECK_THROW(test_fosphor->set_property<int>("wf_mode", 3), uhd::value_error);

    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("wf_predivision_ratio", 200), uhd::value_error);

    BOOST_CHECK_THROW(
        test_fosphor->set_property<int>("wf_decimation", 300), uhd::value_error);
}

BOOST_FIXTURE_TEST_CASE(fosphor_test_graph, fosphor_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port0_info, edge_port1_info;
    edge_port0_info.src_port        = 0;
    edge_port0_info.dst_port        = 0;
    edge_port0_info.is_forward_edge = true;
    edge_port0_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;
    edge_port1_info.src_port        = 1;
    edge_port1_info.dst_port        = 1;
    edge_port1_info.is_forward_edge = true;
    edge_port1_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_radio_node_t mock_radio_block{0};
    mock_ddc_node_t mock_ddc_block{};
    mock_terminator_t mock_sink_term(2, {}, "MOCK_SINK");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    node_accessor.init_props(&mock_ddc_block);
    mock_sink_term.set_edge_property<std::string>(
        "type", "u8", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<std::string>(
        "type", "u8", {res_source_info::INPUT_EDGE, 1});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_radio_block, &mock_ddc_block, edge_port0_info);
    graph.connect(&mock_ddc_block, test_fosphor.get(), edge_port0_info);
    graph.connect(test_fosphor.get(), &mock_sink_term, edge_port0_info);
    graph.connect(test_fosphor.get(), &mock_sink_term, edge_port1_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}
