//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

    double result_rate = node_A->get_tick_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_both_ways_search)
{
    const double test_rate = 0.25;
    MAKE_TICK_SETTING_NODE(node_A, tick_node_ctrl::RATE_UNDEFINED);
    MAKE_TICK_NODE(node_B);
    MAKE_TICK_SETTING_NODE(node_C, test_rate);

    std::cout << "a->b" << std::endl;
    connect_nodes(node_A, node_B);
    std::cout << "b->a" << std::endl;
    connect_nodes(node_B, node_C);

    std::cout << "search" << std::endl;
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

    BOOST_CHECK_THROW(node_B->get_tick_rate(), uhd::runtime_error);
}
