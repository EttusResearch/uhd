//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/node.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using uhd::rfnoc::detail::graph_t;
using namespace uhd::rfnoc;

namespace uhd { namespace rfnoc { namespace detail {

/*! Helper class to access internals of detail::graph
 *
 * This is basically a cheat code to get around the 'private' part of graph_t.
 */
class graph_accessor_t
{
public:
    using vertex_descriptor = graph_t::rfnoc_graph_t::vertex_descriptor;

    graph_accessor_t(graph_t* graph_ptr) : _graph_ptr(graph_ptr)
    { /* nop */
    }

    graph_t::rfnoc_graph_t& get_graph()
    {
        return _graph_ptr->_graph;
    }

    template <typename VertexIterator>
    graph_t::node_ref_t get_node_ref_from_iterator(VertexIterator it)
    {
        return boost::get(graph_t::vertex_property_t(), get_graph(), *it);
    }

    auto find_neighbour(vertex_descriptor origin, res_source_info port_info)
    {
        return _graph_ptr->_find_neighbour(origin, port_info);
    }

    auto find_dirty_nodes()
    {
        return _graph_ptr->_find_dirty_nodes();
    }

    auto get_topo_sorted_nodes()
    {
        return _graph_ptr->_vertices_to_nodes(_graph_ptr->_get_topo_sorted_nodes());
    }

private:
    graph_t* _graph_ptr;
};

}}}; // namespace uhd::rfnoc::detail

BOOST_AUTO_TEST_CASE(test_graph)
{
    graph_t graph{};
    uhd::rfnoc::detail::graph_accessor_t graph_accessor(&graph);
    node_accessor_t node_accessor{};

    auto& bgl_graph = graph_accessor.get_graph();

    // Define some mock nodes:
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio);
    node_accessor.init_props(&mock_tx_radio);

    // In this simple graph, all connections are identical from an edge info
    // perspective, so we're lazy and share an edge_info object:
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = true;
    edge_info.edge            = uhd::rfnoc::detail::graph_t::graph_edge_t::DYNAMIC;

    // Now create the graph:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);

    // A whole bunch of low-level checks first:
    BOOST_CHECK_EQUAL(boost::num_vertices(bgl_graph), 2);
    auto vertex_iterators = boost::vertices(bgl_graph);
    auto vertex_iterator  = vertex_iterators.first;
    auto rx_descriptor    = *vertex_iterator;
    graph_t::node_ref_t node_ref =
        graph_accessor.get_node_ref_from_iterator(vertex_iterator++);
    BOOST_CHECK_EQUAL(node_ref->get_unique_id(), mock_rx_radio.get_unique_id());
    auto tx_descriptor = *vertex_iterator;
    node_ref           = graph_accessor.get_node_ref_from_iterator(vertex_iterator++);
    BOOST_CHECK_EQUAL(node_ref->get_unique_id(), mock_tx_radio.get_unique_id());
    BOOST_CHECK(vertex_iterator == vertex_iterators.second);

    auto rx_neighbour_info =
        graph_accessor.find_neighbour(rx_descriptor, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_REQUIRE(rx_neighbour_info.first);
    BOOST_CHECK_EQUAL(
        rx_neighbour_info.first->get_unique_id(), mock_tx_radio.get_unique_id());
    BOOST_CHECK(
        std::tie(rx_neighbour_info.second.src_port,
            rx_neighbour_info.second.dst_port,
            rx_neighbour_info.second.is_forward_edge)
        == std::tie(edge_info.src_port, edge_info.dst_port, edge_info.is_forward_edge));

    auto tx_neighbour_info =
        graph_accessor.find_neighbour(tx_descriptor, {res_source_info::INPUT_EDGE, 0});
    BOOST_REQUIRE(tx_neighbour_info.first);
    BOOST_CHECK_EQUAL(
        tx_neighbour_info.first->get_unique_id(), mock_rx_radio.get_unique_id());
    BOOST_CHECK(
        std::tie(tx_neighbour_info.second.src_port,
            tx_neighbour_info.second.dst_port,
            tx_neighbour_info.second.is_forward_edge)
        == std::tie(edge_info.src_port, edge_info.dst_port, edge_info.is_forward_edge));

    auto rx_upstream_neighbour_info =
        graph_accessor.find_neighbour(rx_descriptor, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK(rx_upstream_neighbour_info.first == nullptr);
    auto tx_downstream_neighbour_info =
        graph_accessor.find_neighbour(tx_descriptor, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK(tx_downstream_neighbour_info.first == nullptr);
    auto rx_wrongport_neighbour_info =
        graph_accessor.find_neighbour(rx_descriptor, {res_source_info::OUTPUT_EDGE, 1});
    BOOST_CHECK(rx_wrongport_neighbour_info.first == nullptr);
    auto tx_wrongport_neighbour_info =
        graph_accessor.find_neighbour(tx_descriptor, {res_source_info::INPUT_EDGE, 1});
    BOOST_CHECK(tx_wrongport_neighbour_info.first == nullptr);

    // Check there are no dirty nodes (init_props() will clean them all)
    BOOST_CHECK_EQUAL(graph_accessor.find_dirty_nodes().empty(), true);

    auto topo_sorted_nodes = graph_accessor.get_topo_sorted_nodes();
    BOOST_CHECK_EQUAL(topo_sorted_nodes.size(), 2);
    BOOST_CHECK_EQUAL(
        topo_sorted_nodes.at(0)->get_unique_id(), mock_rx_radio.get_unique_id());

    // Now initialize the graph (will force a call to resolve_all_properties())
    graph.commit();

    // This will be ignored
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    BOOST_CHECK_EQUAL(boost::num_vertices(bgl_graph), 2);

    BOOST_REQUIRE_EQUAL(graph.enumerate_edges().size(), 1);
    auto edge0_info = graph.enumerate_edges().at(0);
    BOOST_CHECK_EQUAL(edge0_info.src_blockid, "MOCK_RADIO0");
    BOOST_CHECK_EQUAL(edge0_info.src_port, 0);
    BOOST_CHECK_EQUAL(edge0_info.dst_blockid, "MOCK_RADIO1");
    BOOST_CHECK_EQUAL(edge0_info.dst_port, 0);

    // Now attempt illegal connections (they must all fail)
    edge_info.src_port = 1;
    edge_info.dst_port = 0;
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info), uhd::rfnoc_error);
    edge_info.src_port = 0;
    edge_info.dst_port = 1;
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info), uhd::rfnoc_error);
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = false;
    BOOST_REQUIRE_THROW(
        graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info), uhd::rfnoc_error);
    BOOST_CHECK_EQUAL(graph.enumerate_edges().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_graph_unresolvable)
{
    graph_t graph{};
    node_accessor_t node_accessor{};

    // Define some mock nodes:
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio);
    node_accessor.init_props(&mock_tx_radio);

    // In this simple graph, all connections are identical from an edge info
    // perspective, so we're lazy and share an edge_info object:
    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info(
        0, 0, graph_t::graph_edge_t::DYNAMIC, true);

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    graph.commit();

    // Now set a property that will cause the graph to fail to resolve:
    BOOST_REQUIRE_THROW(mock_tx_radio.set_property<double>("master_clock_rate", 100e6, 0),
        uhd::resolve_error);

    // Now we add a back-edge
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = false;
    graph.connect(&mock_tx_radio, &mock_rx_radio, edge_info);
    UHD_LOG_INFO("TEST", "Testing back edge error path");
    mock_tx_radio.disable_samp_out_resolver = true;
    // The set_property would be valid if we hadn't futzed with the back-edge
    BOOST_REQUIRE_THROW(mock_tx_radio.set_property<double>("master_clock_rate", 200e6, 0),
        uhd::resolve_error);
    UHD_LOG_INFO("TEST", "^^^ Expected ERROR here.");
}

BOOST_AUTO_TEST_CASE(test_graph_disconnect_reconnect)
{
    graph_t graph{};
    node_accessor_t node_accessor{};

    // Define some mock nodes:
    // Source radio
    mock_radio_node_t mock_rx_radio(0);
    // Sink radio
    mock_radio_node_t mock_tx_radio(1);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio);
    node_accessor.init_props(&mock_tx_radio);

    uhd::rfnoc::detail::graph_t::graph_edge_t edge_info(
        0, 0, graph_t::graph_edge_t::DYNAMIC, true);

    // Now create the graph and commit:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    graph.commit();

    BOOST_CHECK_EQUAL(graph.enumerate_edges().size(), 1);

    // disconnect:
    graph.disconnect(&mock_rx_radio, &mock_tx_radio, edge_info);
    graph.release();

    BOOST_CHECK_EQUAL(graph.enumerate_edges().size(), 0);

    // Reconnect:
    graph.connect(&mock_rx_radio, &mock_tx_radio, edge_info);
    graph.commit();

    BOOST_CHECK_EQUAL(graph.enumerate_edges().size(), 1);
}
