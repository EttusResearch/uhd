//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "graph.hpp"
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

class source_node : public test_node
{
public:
    typedef boost::shared_ptr<source_node> sptr;

    source_node(const std::string &test_id, size_t output_port)
        : test_node(test_id)
        , active_rx_streamer_on_port(0)
        , _output_port(output_port) {};

    void set_rx_streamer(bool active, const size_t port)
    {
        if (active) {
            std::cout << "[source_node] Someone is registering a rx streamer on port " << port << std::endl;
            active_rx_streamer_on_port = port;
        }
    }
    size_t active_rx_streamer_on_port;

protected:
    size_t _request_output_port(
            const size_t,
            const uhd::device_addr_t &
    ) const {
        return _output_port;
    }

    const size_t _output_port;

}; /* class result_node */

class sink_node : public test_node
{
public:
    typedef boost::shared_ptr<sink_node> sptr;

    sink_node(const std::string &test_id, size_t input_port)
        : test_node(test_id)
        , active_tx_streamer_on_port(0)
        , _input_port(input_port) {};

    void set_tx_streamer(bool active, const size_t port)
    {
        if (active) {
            std::cout << "[sink_node] Someone is registering a tx streamer on port " << port << std::endl;
            active_tx_streamer_on_port = port;
        }
    }
    size_t active_tx_streamer_on_port;

protected:
    size_t _request_input_port(
            const size_t,
            const uhd::device_addr_t &
    ) const {
        return _input_port;
    }

    const size_t _input_port;

}; /* class result_node */

#define MAKE_SOURCE_NODE(name, port) source_node::sptr name(new source_node(#name, port));
#define MAKE_SINK_NODE(name, port) sink_node::sptr name(new sink_node(#name, port));

BOOST_AUTO_TEST_CASE(test_simple_connect)
{
    MAKE_SOURCE_NODE(node_A, 42);
    MAKE_SINK_NODE(node_B, 23);

    size_t src_port = node_A->connect_downstream(node_B, 1);
    size_t dst_port = node_B->connect_upstream(node_A, 2);

    BOOST_CHECK_EQUAL(src_port, 42);
    BOOST_CHECK_EQUAL(dst_port, 23);

    node_A->set_downstream_port(src_port, dst_port);
    node_B->set_upstream_port(dst_port, src_port);
    BOOST_CHECK_EQUAL(node_A->get_downstream_port(src_port), dst_port);
    BOOST_CHECK_EQUAL(node_B->get_upstream_port(dst_port), src_port);

    BOOST_REQUIRE_THROW(node_A->get_downstream_port(999), uhd::value_error);
}

BOOST_AUTO_TEST_CASE(test_fail)
{
    MAKE_SOURCE_NODE(node_A, 42);
    MAKE_SINK_NODE(node_B, ANY_PORT);

    node_A->connect_downstream(node_B, 1);
    BOOST_REQUIRE_THROW(node_B->connect_upstream(node_A, 2), uhd::type_error);
}

BOOST_AUTO_TEST_CASE(test_set_streamers)
{
    MAKE_SOURCE_NODE(node_A, 0);
    MAKE_NODE(node_B);
    MAKE_SINK_NODE(node_C, 0);

    size_t src_port_A = node_A->connect_downstream(node_B, 0);
    size_t src_port_B = node_B->connect_downstream(node_C, 0);
    size_t dst_port_B = node_B->connect_upstream(node_A, 0);
    size_t dst_port_C = node_C->connect_upstream(node_B, 0);

    std::cout << "src_port_A: " << src_port_A << std::endl;
    std::cout << "src_port_B: " << src_port_B << std::endl;
    std::cout << "dst_port_B: " << dst_port_B << std::endl;
    std::cout << "dst_port_C: " << dst_port_C << std::endl;

    node_A->set_downstream_port(src_port_A, dst_port_B);
    node_B->set_upstream_port(dst_port_B, src_port_A);
    node_B->set_downstream_port(src_port_B, dst_port_C);
    node_C->set_upstream_port(dst_port_C, src_port_B);

    node_A->set_tx_streamer(true, 0);
    node_C->set_rx_streamer(true, 0);

    BOOST_CHECK_EQUAL(node_A->active_rx_streamer_on_port, src_port_A);
    BOOST_CHECK_EQUAL(node_C->active_tx_streamer_on_port, dst_port_C);
}
