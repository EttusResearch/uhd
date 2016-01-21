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
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

class source_node : public test_node
{
public:
    typedef boost::shared_ptr<source_node> sptr;

    source_node(const std::string &test_id, size_t output_port) : test_node(test_id), _output_port(output_port) {};

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

    sink_node(const std::string &test_id, size_t input_port) : test_node(test_id), _input_port(input_port) {};

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

    size_t src_port = node_A->connect_downstream(node_B, 1);
    BOOST_REQUIRE_THROW(node_B->connect_upstream(node_A, 2), uhd::type_error);
}
