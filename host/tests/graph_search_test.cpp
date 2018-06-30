//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "graph.hpp"
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// test class derived, this is what we search for
class result_node : public test_node
{
public:
    typedef boost::shared_ptr<result_node> sptr;

    result_node(const std::string &test_id) : test_node(test_id) {};

}; /* class result_node */

#define MAKE_RESULT_NODE(name) result_node::sptr name(new result_node(#name));

BOOST_AUTO_TEST_CASE(test_simplest_downstream_search)
{
    MAKE_NODE(node_A);
    MAKE_NODE(node_B);

    // Simplest possible scenario: Connect B downstream of A and let
    // A find B
    connect_nodes(node_A, node_B);

    test_node::sptr result = node_A->find_downstream_node<test_node>()[0];
    BOOST_REQUIRE(result);
    BOOST_CHECK_EQUAL(result->get_test_id(), "node_B");
}

BOOST_AUTO_TEST_CASE(test_simple_downstream_search)
{
    MAKE_NODE(node_A);
    MAKE_NODE(node_B0);
    MAKE_NODE(node_B1);

    // Simple scenario: Connect both B{1,2} downstream of A and let
    // it find them
    connect_nodes(node_A, node_B0);
    connect_nodes(node_A, node_B1);

    // We're still searching for test_node, so any downstream block will match
    std::vector< test_node::sptr > result = node_A->find_downstream_node<test_node>();
    BOOST_REQUIRE(result.size() == 2);
    BOOST_CHECK(
            (result[0]->get_test_id() == "node_B0" and result[1]->get_test_id() == "node_B1") or
            (result[1]->get_test_id() == "node_B0" and result[0]->get_test_id() == "node_B1")
    );
    BOOST_CHECK(result[0] == node_B0 or result[0] == node_B1);
}

BOOST_AUTO_TEST_CASE(test_linear_downstream_search)
{
    MAKE_NODE(node_A);
    MAKE_RESULT_NODE(node_B);
    MAKE_RESULT_NODE(node_C);

    // Slightly more complex graph:
    connect_nodes(node_A, node_B);
    connect_nodes(node_B, node_C);

    // This time, we search for result_node
    std::vector< result_node::sptr > result = node_A->find_downstream_node<result_node>();
    std::cout << "size: " << result.size() << std::endl;
    BOOST_CHECK_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0]->get_test_id(), "node_B");
    for(const result_node::sptr &node:  result) {
        std::cout << node->get_test_id() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_multi_iter_downstream_search)
{
    MAKE_NODE(node_A);
    MAKE_NODE(node_B0);
    MAKE_NODE(node_B1);
    MAKE_NODE(node_C0);
    MAKE_RESULT_NODE(node_C1);
    MAKE_RESULT_NODE(node_C2);
    MAKE_RESULT_NODE(node_C3);
    MAKE_RESULT_NODE(node_D0);

    // Slightly more complex graph:
    connect_nodes(node_A, node_B0);
    connect_nodes(node_A, node_B1);
    connect_nodes(node_B0, node_C0);
    connect_nodes(node_B0, node_C1);
    connect_nodes(node_B1, node_C2);
    connect_nodes(node_B1, node_C3);
    connect_nodes(node_C0, node_D0);

    // This time, we search for result_node
    std::vector< result_node::sptr > result = node_A->find_downstream_node<result_node>();
    BOOST_REQUIRE(result.size() == 4);
    for(const result_node::sptr &node:  result) {
        std::cout << node->get_test_id() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_multi_iter_cycle_downstream_search)
{
    MAKE_NODE(node_A);
    MAKE_NODE(node_B0);
    MAKE_NODE(node_B1);
    MAKE_NODE(node_C0);
    MAKE_RESULT_NODE(node_C1);
    MAKE_RESULT_NODE(node_C2);
    MAKE_RESULT_NODE(node_C3);
    MAKE_RESULT_NODE(node_D0);

    // Slightly more complex graph:
    connect_nodes(node_A, node_B0);
    // This connection goes both ways, causing a cycle
    connect_nodes(node_A, node_B1); connect_nodes(node_B1, node_A);
    connect_nodes(node_B0, node_C0);
    connect_nodes(node_B0, node_C1);
    connect_nodes(node_B1, node_C2);
    connect_nodes(node_B1, node_C3);
    connect_nodes(node_C0, node_D0);

    // This time, we search for result_node
    std::vector< result_node::sptr > result = node_A->find_downstream_node<result_node>();
    BOOST_REQUIRE(result.size() == 4);
    for(const result_node::sptr &node:  result) {
        std::cout << node->get_test_id() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_mini_cycle_downstream_and_upstream)
{
    MAKE_NODE(node_A);
    MAKE_NODE(node_B);

    // Connect them in a loop
    connect_nodes(node_A, node_B); connect_nodes(node_B, node_A);

    std::vector< test_node::sptr > result;
    result = node_A->find_downstream_node<test_node>();
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE(result[0] == node_B);
    result = node_B->find_downstream_node<test_node>();
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE(result[0] == node_A);
    result = node_A->find_upstream_node<test_node>();
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE(result[0] == node_B);
    result = node_B->find_upstream_node<test_node>();
    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_REQUIRE(result[0] == node_A);
}
