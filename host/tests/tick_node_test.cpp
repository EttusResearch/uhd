//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "graph.hpp"
#include <uhd/rfnoc/tick_node_ctrl.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// test class derived, knows about rates
class tick_aware_node : public test_node, public tick_node_ctrl
{
public:
    typedef boost::shared_ptr<tick_aware_node> sptr;

    tick_aware_node(const std::string &test_id) : test_node(test_id) {};

}; /* class tick_aware_node */

// test class derived, sets rates
class tick_setting_node : public test_node, public tick_node_ctrl
{
public:
    typedef boost::shared_ptr<tick_setting_node> sptr;

    tick_setting_node(const std::string &test_id, double tick_rate) : test_node(test_id), _tick_rate(tick_rate) {};

protected:
    double _get_tick_rate() { return _tick_rate; };

private:
    const double _tick_rate;

}; /* class tick_setting_node */

#define MAKE_TICK_NODE(name) tick_aware_node::sptr name(new tick_aware_node(#name));
#define MAKE_TICK_SETTING_NODE(name, rate) tick_setting_node::sptr name(new tick_setting_node(#name, rate));

BOOST_AUTO_TEST_CASE(test_simplest_downstream_search)
{
    const double test_rate = 0.25;
    MAKE_TICK_NODE(node_A);
    MAKE_TICK_SETTING_NODE(node_B, test_rate);

    // Simplest possible scenario: Connect B downstream of A and let
    // it find B
    connect_nodes(node_A, node_B);
    node_A->set_tx_streamer(true, 0);
    node_B->set_rx_streamer(true, 0);

    double result_rate = node_A->get_tick_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_both_ways_search)
{
    const double test_rate = 0.25;
    MAKE_TICK_SETTING_NODE(node_A, tick_node_ctrl::RATE_UNDEFINED);
    MAKE_TICK_NODE(node_B);
    MAKE_TICK_SETTING_NODE(node_C, test_rate);

    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);
    node_A->set_tx_streamer(true, 0);
    node_B->set_tx_streamer(true, 0);
    node_B->set_rx_streamer(true, 0);
    node_C->set_rx_streamer(true, 0);

    double result_rate = node_B->get_tick_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_both_ways_search_reversed)
{
    const double test_rate = 0.25;
    MAKE_TICK_SETTING_NODE(node_A, test_rate);
    MAKE_TICK_NODE(node_B);
    MAKE_TICK_SETTING_NODE(node_C, tick_node_ctrl::RATE_UNDEFINED);

    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);
    node_A->set_tx_streamer(true, 0);
    node_B->set_tx_streamer(true, 0);
    node_B->set_rx_streamer(true, 0);
    node_C->set_rx_streamer(true, 0);

    double result_rate = node_B->get_tick_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_both_ways_search_fail)
{
    const double test_rate = 0.25;
    MAKE_TICK_SETTING_NODE(node_A, test_rate);
    MAKE_TICK_NODE(node_B);
    MAKE_TICK_SETTING_NODE(node_C, 2 * test_rate);

    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);
    node_A->set_tx_streamer(true, 0);
    node_B->set_tx_streamer(true, 0);
    node_B->set_rx_streamer(true, 0);
    node_C->set_rx_streamer(true, 0);

    BOOST_CHECK_THROW(node_B->get_tick_rate(), uhd::runtime_error);
}
