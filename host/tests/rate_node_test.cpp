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
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// test class derived, knows about rates
class rate_aware_node : public test_node, public rate_node_ctrl
{
public:
    typedef boost::shared_ptr<rate_aware_node> sptr;

    rate_aware_node(const std::string &test_id) : test_node(test_id) {};

}; /* class rate_aware_node */

// test class derived, sets rates
class rate_setting_node : public test_node, public rate_node_ctrl
{
public:
    typedef boost::shared_ptr<rate_setting_node> sptr;

    rate_setting_node(const std::string &test_id, double samp_rate) : test_node(test_id), _samp_rate(samp_rate) {};

    double get_input_samp_rate(size_t) { return _samp_rate; };
    double get_output_samp_rate(size_t) { return _samp_rate; };

private:
    double _samp_rate;

}; /* class rate_setting_node */

#define MAKE_RATE_NODE(name) rate_aware_node::sptr name(new rate_aware_node(#name));
#define MAKE_RATE_SETTING_NODE(name, rate) rate_setting_node::sptr name(new rate_setting_node(#name, rate));

BOOST_AUTO_TEST_CASE(test_simplest_downstream_search)
{
    const double test_rate = 0.25;
    MAKE_RATE_NODE(node_A);
    MAKE_RATE_SETTING_NODE(node_B, test_rate);

    // Simplest possible scenario: Connect B downstream of A and let
    // it find B
    connect_nodes(node_A, node_B);

    double result_rate = node_A->get_input_samp_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_skip_downstream_search)
{
    const double test_rate = 0.25;
    MAKE_RATE_NODE(node_A);
    MAKE_NODE(node_B);
    MAKE_RATE_SETTING_NODE(node_C, test_rate);

    // Slightly more elaborate: Add another block in between that has no
    // clue about rates
    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);

    double result_rate = node_A->get_input_samp_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_tree_downstream_search)
{
    const double test_rate = 0.25;
    MAKE_RATE_NODE(node_A);
    MAKE_NODE(node_B0);
    MAKE_RATE_SETTING_NODE(node_B1, test_rate);
    MAKE_RATE_SETTING_NODE(node_C0, test_rate);
    MAKE_RATE_SETTING_NODE(node_C1, rate_node_ctrl::RATE_UNDEFINED);

    // Tree: Downstream of our first node are 3 rate setting blocks.
    // Two set the same rate, the third does not care.
    connect_nodes(node_A, node_B0);
    connect_nodes(node_A, node_B1);
    connect_nodes(node_B0, node_C0);
    connect_nodes(node_B0, node_C1);

    double result_rate = node_A->get_input_samp_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

BOOST_AUTO_TEST_CASE(test_tree_downstream_search_throw)
{
    const double test_rate = 0.25;
    MAKE_RATE_NODE(node_A);
    MAKE_NODE(node_B0);
    MAKE_RATE_SETTING_NODE(node_B1, test_rate);
    MAKE_RATE_SETTING_NODE(node_C0, test_rate);
    MAKE_RATE_SETTING_NODE(node_C1, test_rate * 2);

    // Tree: Downstream of our first node are 3 rate setting blocks.
    // Two set the same rate, the third has a different rate.
    // This will cause a throw.
    connect_nodes(node_A, node_B0);
    connect_nodes(node_A, node_B1);
    connect_nodes(node_B0, node_C0);
    connect_nodes(node_B0, node_C1);

    BOOST_CHECK_THROW(node_A->get_input_samp_rate(), uhd::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_skip_upstream_search)
{
    const double test_rate = 0.25;
    MAKE_RATE_SETTING_NODE(node_A, test_rate);
    MAKE_NODE(node_B);
    MAKE_RATE_NODE(node_C);

    // Slightly more elaborate: Add another block in between that has no
    // clue about rates
    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);

    double result_rate = node_C->get_output_samp_rate();
    BOOST_CHECK_EQUAL(result_rate, test_rate);
}

