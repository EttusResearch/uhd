//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_graph_mock_nodes.hpp"
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

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

    std::string get_unique_id() const
    {
        return "MOCK_INVALID_NODE1";
    }

    size_t get_num_input_ports() const
    {
        return 1;
    }

    size_t get_num_output_ports() const
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

    size_t get_num_input_ports() const
    {
        return 1;
    }

    size_t get_num_output_ports() const
    {
        return 1;
    }

    std::string get_unique_id() const
    {
        return "MOCK_INVALID_NODE2";
    }

    // When we change this, we break resolver #2.
    double factor = 2.0;

private:
    property_t<double> _in{"in", 1.0, {res_source_info::INPUT_EDGE}};
    property_t<double> _out{"out", 2.0, {res_source_info::OUTPUT_EDGE}};
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
    graph.initialize();
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 1);

    mock_tx_radio.set_property<double>("master_clock_rate", 100e6, 0);
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 2);

    UHD_LOG_INFO("TEST", "Now tempting DDC to invalid prop value...");
    mock_ddc.set_property<int>("decim", 42, 0);
    // It will bounce back:
    BOOST_CHECK_EQUAL(mock_ddc._decim.get(), 2);
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
    BOOST_REQUIRE_THROW(graph.initialize(), uhd::resolve_error);
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
    graph.initialize();

    const size_t rx_rssi_resolver_count = mock_rx_radio.rssi_resolver_count;
    UHD_LOG_DEBUG("TEST", "RX RSSI: " << mock_rx_radio.get_property<double>("rssi"));
    // The next value must match the value in graph.cpp
    constexpr size_t MAX_NUM_ITERATIONS = 2;
    BOOST_CHECK_EQUAL(
        rx_rssi_resolver_count + MAX_NUM_ITERATIONS, mock_rx_radio.rssi_resolver_count);
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
    graph.initialize();
}
