//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/topo_graph.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>


using namespace uhd::rfnoc::detail;

BOOST_AUTO_TEST_CASE(topo_node_check_ops)
{
    topo_node_t sep0;
    sep0.device_id = 2;
    sep0.type = topo_node_t::node_type::STRM_EP;
    sep0.inst = 0;
    sep0.extended_info = 12;

    topo_node_t sep1;
    sep1.device_id = 2;
    sep1.type = topo_node_t::node_type::STRM_EP;
    sep1.inst = 0;
    sep1.extended_info = 23;

    BOOST_CHECK(sep0 == sep1);

    sep1.inst = sep0.inst + 1;
    BOOST_CHECK(sep0 != sep1);
    BOOST_CHECK(sep0 < sep1);

    BOOST_CHECK_EQUAL(sep0.to_string(), "device:2/sep:0");
}


BOOST_AUTO_TEST_CASE(topo_graph_add_node)
{
    topo_graph_t g;

    topo_node_t sep0;
    sep0.device_id = 1;
    sep0.type = topo_node_t::node_type::STRM_EP;
    sep0.inst = 0;

    BOOST_CHECK(g.add_node(sep0));
    BOOST_CHECK(!g.add_node(sep0));

    topo_edge_t edge0; // From sep0
    edge0.type = topo_edge_t::edge_type::HOST;
    edge0.src_port = 0; // Need to not use -1 ("don't care")
    topo_node_t ta0;
    ta0.device_id = 2;
    ta0.type = topo_node_t::node_type::XPORT;
    ta0.inst = 0;

    // Can't add a route from ta0, because it's not yet in the graph
    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    BOOST_REQUIRE_THROW(g.add_edge(ta0, sep0, edge0), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");

    // This should work though:
    BOOST_CHECK(g.add_edge(sep0, ta0, edge0));
    auto edge_check = g.get_edge(sep0, ta0);
    BOOST_CHECK(edge_check.type == topo_edge_t::edge_type::HOST);

    // But only once:
    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    BOOST_REQUIRE_THROW(g.add_edge(sep0, ta0, edge0), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");

    // This is untypical, but we want to test adding new routes for existing
    // nodes:
    topo_node_t xbar0;
    xbar0.device_id = 3;
    xbar0.type      = topo_node_t::node_type::XBAR;
    xbar0.inst      = 0;
    topo_node_t xbar1;
    xbar1.device_id = 3;
    xbar1.type      = topo_node_t::node_type::XBAR;
    xbar1.inst      = 1;

    BOOST_CHECK(g.add_node(xbar0));
    topo_edge_t edge1; // From xbar0 to xbar1
    edge1.type = topo_edge_t::edge_type::ON_CHIP;
    edge1.src_port = 2;
    topo_edge_t edge2; // Also from xbar0 to xbar1
    edge2.type = topo_edge_t::edge_type::ON_CHIP;
    edge2.src_port = 2;

    BOOST_CHECK(g.add_edge(xbar0, xbar1, edge1));
    // Can't add same route twice
    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    BOOST_REQUIRE_THROW(g.add_edge(xbar0, xbar1, edge2), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");
    edge2.src_port++;
    // Note: The ! here is because we already added xbar2 earlier.
    BOOST_CHECK(!g.add_edge(xbar0, xbar1, edge2));

    BOOST_CHECK(g.has_route(sep0, ta0));
    BOOST_CHECK(!g.has_route(sep0, xbar0));

    auto filtered_nodes = g.get_nodes([&](const topo_node_t& node) {
        return node.type == sep0.type && node.device_id == sep0.device_id;
    });
    BOOST_CHECK_EQUAL(filtered_nodes.size(), 1);
    BOOST_CHECK(filtered_nodes.front() == sep0);

    using uhd::rfnoc::sep_id_t;
    BOOST_CHECK_EQUAL(g.get_nodes([&](const topo_node_t& node) {
                           return node.epid == sep_id_t(23);
                       }).size(),
        0);

    g.access_node(sep0).epid = 23;

    BOOST_CHECK_EQUAL(g.get_nodes([&](const topo_node_t& node) {
                           return node.epid == sep_id_t(23);
                       }).size(),
        1);
}

BOOST_AUTO_TEST_CASE(topo_graph_update_edge)
{
    topo_graph_t g;

    topo_node_t sep0;
    sep0.device_id = 1;
    sep0.type = topo_node_t::node_type::STRM_EP;
    sep0.inst = 0;

    topo_edge_t edge0; // From sep0
    edge0.type = topo_edge_t::edge_type::HOST;
    edge0.src_port = 0;

    topo_node_t ta0;
    ta0.device_id = 2;
    ta0.type = topo_node_t::node_type::XPORT;
    ta0.inst = 0;

    BOOST_CHECK(g.add_node(sep0));
    BOOST_CHECK(g.add_edge(sep0, ta0, edge0));

    BOOST_CHECK_EQUAL(g.get_distance(sep0, ta0), 1);
    g.update_weight(sep0, edge0.src_port, 5);
    BOOST_CHECK_EQUAL(g.get_distance(sep0, ta0), 5);
}


BOOST_AUTO_TEST_CASE(topo_graph_find_route)
{
    /**************************************************************************
     * We will construct the following graph:
     *
     * SEP0 -\
     *  |     \ (1)
     * XBAR0  |
     *  |    XBAR2   SEP3 --> SEP4
     * XBAR1  |
     *  |     /
     * SEP1 -/
     *
     *
     * Our job is to find shortest route from SEP0 to SEP1, which goes through
     * xbar2. SEP3 and SEP4 are used to test that disjointed subgraphs won't
     * hurt our path-finding algorithms.
     **************************************************************************/

    topo_graph_t g;

    topo_node_t sep0;
    sep0.device_id = 1;
    sep0.type      = topo_node_t::node_type::STRM_EP;
    sep0.inst      = 0;
    topo_node_t sep1;
    sep1.device_id = 1;
    sep1.type      = topo_node_t::node_type::STRM_EP;
    sep1.inst      = 1;
    topo_node_t sep2;
    sep2.device_id = 1;
    sep2.type      = topo_node_t::node_type::STRM_EP;
    sep2.inst      = 2;
    topo_node_t xbar0;
    xbar0.device_id = 1;
    xbar0.type      = topo_node_t::node_type::XBAR;
    xbar0.inst      = 0;
    topo_node_t xbar1;
    xbar1.device_id = 1;
    xbar1.type      = topo_node_t::node_type::XBAR;
    xbar1.inst      = 1;
    topo_node_t xbar2;
    xbar2.device_id = 1;
    xbar2.type      = topo_node_t::node_type::XBAR;
    xbar2.inst      = 2;
    // The next two are just for distraction
    topo_node_t sep3;
    sep3.device_id = 1;
    sep3.type      = topo_node_t::node_type::STRM_EP;
    sep3.inst      = 3;
    topo_node_t sep4;
    sep4.device_id = 1;
    sep4.type      = topo_node_t::node_type::STRM_EP;
    sep4.inst      = 4;

    topo_edge_t edge; // We'll reuse this
    edge.type = topo_edge_t::edge_type::ON_CHIP;

    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    BOOST_REQUIRE_THROW(g.has_route(sep0, xbar0), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");

    g.add_node(sep0);
    g.add_node(sep3); // Distractor node

    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    // xbar0 not yet added, so this should throw
    BOOST_REQUIRE_THROW(g.has_route(sep0, xbar0), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");

    // Now add all the edges
    edge.src_port = 0;
    edge.dst_port = 0;
    g.add_biedge(sep0, xbar0, edge);
    edge.dst_port = 2;
    edge.src_port = 2;
    g.add_edge(xbar0, xbar1, edge);
    edge.src_port = 0;
    edge.dst_port = 0;
    g.add_edge(xbar1, sep1, edge);
    edge.src_port = 1;
    edge.dst_port = 1;
    g.add_edge(sep0, xbar2, edge);
    edge.src_port = 3;
    edge.dst_port = 3;
    g.add_edge(xbar2, sep1, edge);
    edge.src_port = topo_edge_t::ANY_PORT;
    edge.dst_port = topo_edge_t::ANY_PORT;
    g.add_edge(sep3, sep4, edge); // Distraction route

    BOOST_CHECK(g.has_route(sep0, sep1));
    BOOST_CHECK(!g.has_route(sep0, sep3));

    UHD_LOG_INFO("TEST", "Expect ERROR here VVV:");
    BOOST_REQUIRE_THROW(g.get_route(sep0, sep3), uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expect ERROR here ^^^:");

    // Now check we can find the best route
    auto r = g.get_route(sep0, sep1);
    auto r_it = r.cbegin();
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:0");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/xbar:2");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:1");
    BOOST_CHECK(r_it->edge.type == topo_edge_t::edge_type::END_OF_ROUTE);

    // Check we get the same route if we don't specify source node directly
    r = g.get_best_route(
        // Let's try all other SEPs
        [&](const topo_node_t& node) {
            return node.type == topo_node_t::node_type::STRM_EP && node.inst != 1;
        },
        sep1);
    r_it = r.cbegin();
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:0");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/xbar:2");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:1");
    BOOST_CHECK(r_it->edge.type == topo_edge_t::edge_type::END_OF_ROUTE);

    // Now we update the distance from SEP0 to XBAR2. The new optimal route now
    // goes via XBAR0 and XBAR1
    g.update_weight(sep0, 1, 100);
    r = g.get_route(sep0, sep1);
    r_it = r.cbegin();
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:0");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/xbar:0");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/xbar:1");
    ++r_it;
    BOOST_CHECK_EQUAL(r_it->node.to_string(), "device:1/sep:1");
    BOOST_CHECK(r_it->edge.type == topo_edge_t::edge_type::END_OF_ROUTE);

    std::cout << g.to_dot() << std::endl;

    BOOST_CHECK_EQUAL(g.get_connected_nodes(sep0).size(), 4);
    BOOST_CHECK_EQUAL(g.get_connected_nodes(sep0,
                           [](const topo_node_t& node) {
                               return node.type == topo_node_t::node_type::XBAR;
                           })
                          .size(),
        3);
}
