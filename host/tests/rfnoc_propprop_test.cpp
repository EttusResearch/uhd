//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <algorithm>

/*! Mock invalid node
 *
 * This block has an output prop that is always twice the input prop. This block
 * is invalid because the defaults don't work.
 */
class mock_invalid_node1_t : public node_t
{
public:
    mock_invalid_node1_t()
    {
        register_property(&_in);
        register_property(&_out);

        add_property_resolver({&_in}, {&_out}, [this]() { _out = _in * 2; });
        add_property_resolver({&_out}, {&_in}, [this]() { _in = _out / 2; });
    }

    std::string get_unique_id() const override
    {
        return "MOCK_INVALID_NODE1";
    }

    size_t get_num_input_ports() const override
    {
        return 1;
    }

    size_t get_num_output_ports() const override
    {
        return 1;
    }

private:
    property_t<double> _in{"in", 1.0, {res_source_info::INPUT_EDGE}};
    // This has an invalid default value: It would have to be 2.0 for the block
    // to be able to initialize
    property_t<double> _out{"out", 1.0 /* SIC */, {res_source_info::OUTPUT_EDGE}};
};

/*! Mock invalid node
 *
 * This block will write conflicting values to the output at resolution time.
 */
class mock_invalid_node2_t : public node_t
{
public:
    mock_invalid_node2_t()
    {
        register_property(&_in);
        register_property(&_out);

        add_property_resolver({&_in}, {&_out}, [this]() {
            UHD_LOG_INFO("MOCK2", "Calling resolver 1/2 for _out");
            _out = _in * 2.0;
        });
        // If this->factor != 2.0, then this resolver will contradict the
        // previous one:
        add_property_resolver({&_in}, {&_out}, [this]() {
            UHD_LOG_INFO("MOCK2", "Calling resolver 2/2 for _out");
            _out = _in * this->factor;
        });
        add_property_resolver({&_out}, {&_in}, [this]() {
            UHD_LOG_INFO("MOCK2", "Calling resolver for _in");
            _in = _out / 2.0;
        });
    }

    void mark_in_dirty()
    {
        prop_accessor_t prop_accessor{};
        auto access_lock = prop_accessor.get_scoped_prop_access(_in, property_base_t::RW);
        double old_val   = _in.get();
        _in.set(old_val * 2.0);
        _in.set(old_val);
    }

    size_t get_num_input_ports() const override
    {
        return 1;
    }

    size_t get_num_output_ports() const override
    {
        return 1;
    }

    std::string get_unique_id() const override
    {
        return "MOCK_INVALID_NODE2";
    }

    // When we change this, we break resolver #2.
    double factor = 2.0;

private:
    property_t<double> _in{"in", 1.0, {res_source_info::INPUT_EDGE}};
    property_t<double> _out{"out", 2.0, {res_source_info::OUTPUT_EDGE}};
};

/*! Mock node, circular prop deps
 */
class mock_circular_prop_node_t : public node_t
{
public:
    mock_circular_prop_node_t()
    {
        register_property(&_x1);
        register_property(&_x2);
        register_property(&_x4);

        add_property_resolver({&_x1}, {&_x2}, [this]() {
            RFNOC_LOG_INFO("Calling resolver for _x1");
            _x2 = 2.0 * _x1.get();
        });
        add_property_resolver({&_x2}, {&_x4}, [this]() {
            RFNOC_LOG_INFO("Calling resolver for _x2");
            _x4 = 2.0 * _x2.get();
        });
        add_property_resolver({&_x4}, {&_x1}, [this]() {
            RFNOC_LOG_INFO("Calling resolver for _x4");
            _x1 = _x4.get() / 4.0;
        });
    }

    size_t get_num_input_ports() const override
    {
        return 1;
    }

    size_t get_num_output_ports() const override
    {
        return 1;
    }

    std::string get_unique_id() const override
    {
        return "MOCK_CIRCULAR_PROPS";
    }

    property_t<double> _x1{"x1", 1.0, {res_source_info::USER}};
    property_t<double> _x2{"x2", 2.0, {res_source_info::USER}};
    property_t<double> _x4{"x4", 4.0, {res_source_info::USER}};
};

/*! A mock RFNoC block
 *
 * A mock RFNoC block deriving from noc_block_base, strictly for the purpose
 * of testing multiple calls to set_mtu_forwarding_policy() (which is a
 * protected member of noc_block_base, sigh)
 */
class mock_noc_block_t : public noc_block_base
{
public:
    mock_noc_block_t(noc_block_base::make_args_ptr make_args) :
        noc_block_base(std::move(make_args))
    {
    }

    void set_mtu_forwarding_policy(node_t::forwarding_policy_t policy)
    {
        noc_block_base::set_mtu_forwarding_policy(policy);
    }
};


// Do some sanity checks on the mock just so we don't get surprised later
BOOST_AUTO_TEST_CASE(test_mock)
{
    BOOST_CHECK_EQUAL(1, mock_ddc_node_t::coerce_decim(1));
    BOOST_CHECK_EQUAL(2, mock_ddc_node_t::coerce_decim(2));
    BOOST_CHECK_EQUAL(512, mock_ddc_node_t::coerce_decim(1212));
    BOOST_CHECK_EQUAL(512, mock_ddc_node_t::coerce_decim(513));
    BOOST_CHECK_EQUAL(2, mock_ddc_node_t::coerce_decim(3));

    mock_ddc_node_t mock{};
    BOOST_CHECK(mock._decim.is_dirty());
    BOOST_CHECK(mock._samp_rate_out.is_dirty());
    BOOST_CHECK(mock._samp_rate_in.is_dirty());
    BOOST_CHECK_EQUAL(mock._decim.get(), DEFAULT_DECIM);
    BOOST_CHECK_EQUAL(mock._samp_rate_out.get(), DEFAULT_RATE);
    BOOST_CHECK_EQUAL(mock._samp_rate_in.get(), DEFAULT_RATE);
}

BOOST_AUTO_TEST_CASE(test_init_and_resolve)
{
    mock_ddc_node_t mock_ddc{};
    mock_radio_node_t mock_radio(0);
    node_accessor_t node_accessor{};

    node_accessor.init_props(&mock_ddc);
    node_accessor.init_props(&mock_radio);

    BOOST_CHECK(!mock_ddc._decim.is_dirty());
    BOOST_CHECK(!mock_ddc._samp_rate_out.is_dirty());
    BOOST_CHECK(!mock_ddc._samp_rate_in.is_dirty());
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), DEFAULT_DECIM);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_out.get(), DEFAULT_RATE);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get(), DEFAULT_RATE);

    BOOST_CHECK_EQUAL(mock_ddc.get_property<int>("decim", 0), DEFAULT_DECIM);

    mock_ddc.set_property("decim", 2, 0);
    BOOST_CHECK(!mock_ddc._decim.is_dirty());
    node_accessor.resolve_props(&mock_ddc);

    BOOST_CHECK_EQUAL(mock_ddc.get_property<int>("decim", 0), 2);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get(), DEFAULT_RATE);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get() / 2, mock_ddc._samp_rate_out.get());
}

BOOST_AUTO_TEST_CASE(test_failures)
{
    node_accessor_t node_accessor{};

    UHD_LOG_INFO("TEST", "We expect an ERROR log message next:");
    mock_invalid_node1_t mock1{};
    // BOOST_REQUIRE_THROW(
    // node_accessor.init_props(&mock1),
    // uhd::runtime_error);

    mock_invalid_node2_t mock2{};
    node_accessor.init_props(&mock2);
    mock2.factor = 1.0;
    mock2.mark_in_dirty();
    BOOST_REQUIRE_THROW(node_accessor.resolve_props(&mock2), uhd::resolve_error);
}

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

BOOST_AUTO_TEST_CASE(test_mtu_forwarding_policy_restrictions)
{
    // Most of this is just dummy stuff required to correctly instantiate a
    // noc_block_base-derived block and is inconsequential to the test itself
    mock_block_container mbc;
    mbc.reg_iface = std::make_shared<mock_reg_iface_t>();
    mbc.tree      = uhd::property_tree::make();
    mbc.make_args                   = std::make_unique<noc_block_base::make_args_t>();
    mbc.make_args->noc_id           = 0x01020304;
    mbc.make_args->block_id         = block_id_t("0/Dummy#0");
    mbc.make_args->num_input_ports  = 2;
    mbc.make_args->num_output_ports = 2;
    mbc.make_args->mtu              = 8000;
    mbc.make_args->chdr_w           = uhd::rfnoc::CHDR_W_64;
    mbc.make_args->reg_iface        = mbc.reg_iface;
    mbc.make_args->tree             = mbc.tree;
    mbc.make_args->tb_clk_iface =
        std::make_shared<clock_iface>("dummy");
    mbc.make_args->ctrlport_clk_iface =
        std::make_shared<clock_iface>("dummy");
    mbc.make_args->mb_control = nullptr;

    // Construct the dummy RFNoC block
    mock_noc_block_t mock_block(std::move(mbc.make_args));

    // Set the MTU forwarding policy once; this should work
    mock_block.set_mtu_forwarding_policy(noc_block_base::forwarding_policy_t::ONE_TO_ONE);

    // And the seocnd time should generate an exception
    BOOST_REQUIRE_THROW(mock_block.set_mtu_forwarding_policy(noc_block_base::forwarding_policy_t::ONE_TO_FAN), uhd::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_graph_resolve_ddc_radio)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};
    // Define some mock nodes:
    mock_ddc_node_t mock_ddc{};
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_ddc);
    node_accessor.init_props(&mock_tx_radio);
    node_accessor.init_props(&mock_rx_radio);

    // In this simple graph, all connections are identical from an edge info
    // perspective, so we're lazy and share an edge_info object:
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio, &mock_ddc, edge_info);
    graph.connect(&mock_ddc, &mock_tx_radio, edge_info);
    graph.commit();
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 1);

    mock_tx_radio.set_property<double>("master_clock_rate", 100e6, 0);
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 2);

    UHD_LOG_INFO("TEST", "Now tempting DDC to invalid prop value...");
    mock_ddc.set_property<int>("decim", 42, 0);
    // It will bounce back:
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 2);

    graph.release();
    mock_tx_radio.set_property<double>("master_clock_rate", 200e6, 0);
    // Won't change yet:
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 2);
    graph.commit();
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 1);
}


BOOST_AUTO_TEST_CASE(test_graph_catch_invalid_graph)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};
    // Define some mock nodes:
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_tx_radio);
    node_accessor.init_props(&mock_rx_radio);
    mock_tx_radio.set_property<double>("master_clock_rate", 100e6, 0);

    // In this simple graph, all connections are identical from an edge info
    // perspective, so we're lazy and share an edge_info object:
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    BOOST_REQUIRE_THROW(graph.commit(), uhd::resolve_error);
    UHD_LOG_INFO("TEST", "^^^ Expected an error message.");
}

BOOST_AUTO_TEST_CASE(test_graph_ro_prop)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};
    // Define some mock nodes:
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_tx_radio);
    node_accessor.init_props(&mock_rx_radio);
    BOOST_CHECK_EQUAL(mock_tx_radio.rssi_resolver_count, 1);
    BOOST_CHECK_EQUAL(mock_rx_radio.rssi_resolver_count, 1);

    // In this simple graph, all connections are identical from an edge info
    // perspective, so we're lazy and share an edge_info object:
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    graph.commit();

    const size_t rx_rssi_resolver_count = mock_rx_radio.rssi_resolver_count;
    UHD_LOG_INFO("TEST", "Now testing mock RSSI resolver/get prop");
    UHD_LOG_DEBUG("TEST", "RX RSSI: " << mock_rx_radio.get_property<double>("rssi"));
    // The next value must match the value in graph.cpp. We have one additional
    // backward- and forward resolution.
    BOOST_CHECK_EQUAL(rx_rssi_resolver_count + 2, mock_rx_radio.rssi_resolver_count);
}

BOOST_AUTO_TEST_CASE(test_graph_double_connect)
{
    node_accessor_t node_accessor{};
    using uhd::rfnoc::detail::graph_t;
    graph_t graph{};
    using edge_t = graph_t::graph_edge_t;
    // Define some mock nodes:
    mock_radio_node_t mock_rx_radio0(0);
    mock_radio_node_t mock_rx_radio1(1);
    mock_radio_node_t mock_tx_radio0(2);
    mock_radio_node_t mock_tx_radio1(3);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_tx_radio0);
    node_accessor.init_props(&mock_tx_radio1);
    node_accessor.init_props(&mock_rx_radio0);
    node_accessor.init_props(&mock_rx_radio1);

    graph.connect(&mock_rx_radio0, &mock_tx_radio0, {0, 0, edge_t::DYNAMIC, true});
    // Twice is also OK:
    UHD_LOG_INFO("TEST", "Testing double-connect with same edges");
    graph.connect(&mock_rx_radio0, &mock_tx_radio0, {0, 0, edge_t::DYNAMIC, true});
    UHD_LOG_INFO("TEST", "Testing double-connect with same edges, different attributes");
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio0, &mock_tx_radio0, {0, 0, edge_t::DYNAMIC, false}),
        uhd::rfnoc_error);
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio0, &mock_tx_radio0, {0, 0, edge_t::STATIC, false}),
        uhd::rfnoc_error);
    UHD_LOG_INFO("TEST", "Testing double-connect output port, new dest node");
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio0, &mock_tx_radio1, {0, 0, edge_t::DYNAMIC, true}),
        uhd::rfnoc_error);
    UHD_LOG_INFO("TEST", "Testing double-connect input port, new source node");
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio1, &mock_tx_radio0, {0, 0, edge_t::DYNAMIC, true}),
        uhd::rfnoc_error);
    // Add another valid connection
    graph.connect(&mock_rx_radio1, &mock_tx_radio1, {0, 0, edge_t::DYNAMIC, true});
    UHD_LOG_INFO("TEST", "Testing double-connect output port, existing dest node");
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio0, &mock_tx_radio1, {0, 0, edge_t::DYNAMIC, true}),
        uhd::rfnoc_error);
    UHD_LOG_INFO("TEST", "Testing double-connect input port, existing source node");
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio1, &mock_tx_radio0, {0, 0, edge_t::DYNAMIC, true}),
        uhd::rfnoc_error);
}

BOOST_AUTO_TEST_CASE(test_graph_crisscross_fifo)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};
    // Define some mock nodes:
    // Source radios
    mock_radio_node_t mock_rx_radio0(0); // -> 2
    mock_radio_node_t mock_rx_radio1(1); // -> 3
    // Sink radios
    mock_radio_node_t mock_tx_radio0(2);
    mock_radio_node_t mock_tx_radio1(3);
    // FIFO
    mock_fifo_t mock_fifo(2);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio0);
    node_accessor.init_props(&mock_rx_radio1);
    node_accessor.init_props(&mock_tx_radio0);
    node_accessor.init_props(&mock_tx_radio1);
    node_accessor.init_props(&mock_fifo);

    mock_rx_radio0.set_property<double>("master_clock_rate", 200e6, 0);
    mock_rx_radio1.set_property<double>("master_clock_rate", 100e6, 0);
    mock_tx_radio0.set_property<double>("master_clock_rate", 100e6, 0);
    mock_tx_radio1.set_property<double>("master_clock_rate", 200e6, 0);

    using graph_edge_t = uhd::rfnoc::detail::graph_t::graph_edge_t;

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio0, &mock_fifo, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&mock_rx_radio1, &mock_fifo, {0, 1, graph_edge_t::DYNAMIC, true});
    // Notice how we swap the TX radios
    graph.connect(&mock_fifo, &mock_tx_radio0, {1, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&mock_fifo, &mock_tx_radio1, {0, 0, graph_edge_t::DYNAMIC, true});
    UHD_LOG_INFO("TEST", "Now testing criss-cross prop resolution");
    graph.commit();
}

BOOST_AUTO_TEST_CASE(test_circular_deps)
{
    node_accessor_t node_accessor{};
    // Define some mock nodes:
    // Source radios
    mock_circular_prop_node_t mock_circular_prop_node{};

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_circular_prop_node);

    mock_circular_prop_node.set_property<double>("x1", 5.0, 0);
    BOOST_CHECK_EQUAL(mock_circular_prop_node.get_property<double>("x4"), 4 * 5.0);
}

BOOST_AUTO_TEST_CASE(test_propagation_map)
{
    // Set up the graph
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    constexpr size_t NUM_INPUTS  = 8;
    constexpr size_t NUM_OUTPUTS = 8;

    node_t::forwarding_map_t fwd_map = {
        // input edges 0-3 --> output edges 3-0
        {{res_source_info::INPUT_EDGE, 0}, {{res_source_info::OUTPUT_EDGE, 3}}},
        {{res_source_info::INPUT_EDGE, 1}, {{res_source_info::OUTPUT_EDGE, 2}}},
        {{res_source_info::INPUT_EDGE, 2}, {{res_source_info::OUTPUT_EDGE, 1}}},
        {{res_source_info::INPUT_EDGE, 3}, {{res_source_info::OUTPUT_EDGE, 0}}},
        // input edge 4 --> output edges 4 and 5
        {{res_source_info::INPUT_EDGE, 4},
            {{res_source_info::OUTPUT_EDGE, 4}, {res_source_info::OUTPUT_EDGE, 5}}},
        // input edge 5 --> output edges 6 and 7
        {{res_source_info::INPUT_EDGE, 5},
            {{res_source_info::OUTPUT_EDGE, 6}, {res_source_info::OUTPUT_EDGE, 7}}},
        // input edge 6 no destination (i.e. drop)
        {{res_source_info::INPUT_EDGE, 6}, {}}
        // input edge 7 not in map (i.e. drop)
    };

    mock_edge_node_t input{0, NUM_INPUTS, "MOCK_EDGE_NODE<input>"};
    mock_routing_node_t middle{NUM_INPUTS, NUM_OUTPUTS};
    mock_edge_node_t output{NUM_OUTPUTS, 0, "MOCK_EDGE_NODE<output>"};

    middle.set_prop_forwarding_map(fwd_map);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&input);
    node_accessor.init_props(&middle);
    node_accessor.init_props(&output);

    // Prime the output edge properties on the input block
    for (size_t i = 0; i < NUM_INPUTS; i++) {
        input.set_edge_property<int>("prop", 100 + i, {res_source_info::OUTPUT_EDGE, i});
    }

    using graph_edge_t = uhd::rfnoc::detail::graph_t::graph_edge_t;

    // Connect the nodes in the graph
    for (size_t i = 0; i < NUM_INPUTS; i++) {
        graph.connect(&input, &middle, {i, i, graph_edge_t::DYNAMIC, true});
    }
    for (size_t i = 0; i < NUM_OUTPUTS; i++) {
        graph.connect(&middle, &output, {i, i, graph_edge_t::DYNAMIC, true});
    }
    UHD_LOG_INFO("TEST", "Now testing map-driven property propagation");
    graph.commit();

    // Verify that the properties were propagated per the table
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 0}), 103);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 1}), 102);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 2}), 101);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 3}), 100);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 4}), 104);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 5}), 104);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 6}), 105);
    BOOST_CHECK_EQUAL(
        output.get_edge_property<int>("prop", {res_source_info::INPUT_EDGE, 7}), 105);
}

BOOST_AUTO_TEST_CASE(test_propagation_map_exception_invalid_destination)
{
    // Set up the graph
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    // Create a map that will generate an exception at propagation time due
    // to the mapping pointing to a non-existent port
    node_t::forwarding_map_t no_port_fwd_map = {
        // input edge 0 --> output edge 1 (output port does not exist)
        {{res_source_info::INPUT_EDGE, 0}, {{res_source_info::OUTPUT_EDGE, 1}}}};

    mock_edge_node_t generator{0, 1, "MOCK_EDGE_NODE<generator>"};
    mock_routing_node_t router{1, 1};
    mock_edge_node_t receiver{1, 0, "MOCK_EDGE_NODE<receiver>"};

    router.set_prop_forwarding_map(no_port_fwd_map);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&generator);
    node_accessor.init_props(&router);
    node_accessor.init_props(&receiver);

    generator.set_edge_property<int>("prop", 100, {res_source_info::OUTPUT_EDGE, 0});

    using graph_edge_t = uhd::rfnoc::detail::graph_t::graph_edge_t;

    // Connect the nodes in the graph
    graph.connect(&generator, &router, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&router, &receiver, {0, 0, graph_edge_t::DYNAMIC, true});

    UHD_LOG_INFO("TEST",
        "Now testing map-driven property propagation with invalid map (no destination "
        "port)");
    BOOST_REQUIRE_THROW(graph.commit(), uhd::rfnoc_error);
}

BOOST_AUTO_TEST_CASE(test_graph_node_deletion_resolver_fix)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    // First node: Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Second node: DDC
    mock_ddc_node_t mock_ddc{};

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio);
    node_accessor.init_props(&mock_ddc);

    // The DDC node at this point is not and has never been part of a
    // graph, but setting the decimation property should still invoke
    // that resolver
    constexpr int new_interp_ratio = 100;
    mock_ddc.set_property<int>("decim", new_interp_ratio, 0);
    BOOST_CHECK_EQUAL(
        mock_ddc._samp_rate_in.get() / new_interp_ratio, mock_ddc._samp_rate_out.get());

    // Connect the radio to the DDC in a simple graph
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    graph.connect(&mock_rx_radio, &mock_ddc, edge_info);
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), new_interp_ratio);

    // Set the radio MCR; after graph committal, that should propagate
    // to the DDC input rate
    const int ddc_input_rate = mock_ddc._samp_rate_in.get();
    mock_rx_radio.set_property<double>("master_clock_rate", 100e6, 0);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get(), ddc_input_rate);
    graph.commit();
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get(), 100e6);

    // Now disconect the DDC from the radio--as there are no incoming or
    // outgoing edges on either node afterwards, the nodes should be removed
    // from the graph altogether and thus their resolver callbacks should
    // also be reset to the default behavior
    graph.disconnect(&mock_rx_radio, &mock_ddc, edge_info);

    // Setting a new decimation ratio even after the DDC node has been
    // removed from the graph should work as expected
    constexpr int disconnected_interp_ratio = 20;
    mock_ddc.set_property<int>("decim", disconnected_interp_ratio, 0);
    BOOST_CHECK_EQUAL(mock_ddc._samp_rate_in.get() / disconnected_interp_ratio,
        mock_ddc._samp_rate_out.get());
}


BOOST_AUTO_TEST_CASE(test_graph_node_loop_resolve)
{
    // Mock the radio with only the AIS property. It will keep AIS at a multiple
    // of 4.
    class mock_radio_ais_node_t : public node_t
    {
    public:
        mock_radio_ais_node_t()
        {
            register_property(&_ais_in);
            register_property(&_ais_out);

            add_property_resolver({&_ais_in}, {&_ais_in}, [this]() {
                _ais_in = std::max<size_t>(4, (_ais_in.get() / 4) * 4);
            });
            add_property_resolver({&_ais_out}, {&_ais_out}, [this]() {
                _ais_out = std::max<size_t>(4, (_ais_out.get() / 4) * 4);
            });
        }

        std::string get_unique_id() const override
        {
            return "MOCK_RADIO_AIS_NODE";
        }

        size_t get_num_input_ports() const override
        {
            return 1;
        }

        size_t get_num_output_ports() const override
        {
            return 1;
        }

    private:
        property_t<size_t> _ais_in{
            PROP_KEY_ATOMIC_ITEM_SIZE, 4, {res_source_info::INPUT_EDGE}};
        property_t<size_t> _ais_out{
            PROP_KEY_ATOMIC_ITEM_SIZE, 8, {res_source_info::OUTPUT_EDGE}};
    };

    // Mock the replay with only the AIS property. It will keep AIS at exactly 8.
    class mock_replay_ais_node_t : public node_t
    {
    public:
        mock_replay_ais_node_t()
        {
            register_property(&_ais_in);
            register_property(&_ais_out);

            add_property_resolver({&_ais_in}, {&_ais_in}, [this]() {
                _ais_in = 8;
            });
            add_property_resolver({&_ais_out}, {&_ais_out}, [this]() {
                _ais_out = 8;
            });
        }

        std::string get_unique_id() const override
        {
            return "MOCK_REPLAY_AIS_NODE";
        }

        size_t get_num_input_ports() const override
        {
            return 1;
        }

        size_t get_num_output_ports() const override
        {
            return 1;
        }

    private:
        property_t<size_t> _ais_in{
            PROP_KEY_ATOMIC_ITEM_SIZE, 4, {res_source_info::INPUT_EDGE}};
        property_t<size_t> _ais_out{
            PROP_KEY_ATOMIC_ITEM_SIZE, 8, {res_source_info::OUTPUT_EDGE}};
    };

    // Now let's define things
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    // Create mock blocks
    mock_radio_ais_node_t mock_radio_ais_node{};
    mock_replay_ais_node_t mock_replay_ais_node{};

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_radio_ais_node);
    node_accessor.init_props(&mock_replay_ais_node);

    // Connect the radio to the replay in a simple graph
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    graph.connect(&mock_radio_ais_node, &mock_replay_ais_node, edge_info);
    // Declare back-edge
    edge_info.property_propagation_active = false;
    graph.connect(&mock_replay_ais_node, &mock_radio_ais_node, edge_info);
    UHD_LOG_INFO("TEST", "Committing replay/radio loop graph");
    BOOST_CHECK_NO_THROW(graph.commit(););
}
