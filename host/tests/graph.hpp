//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_TEST_GRAPH_HPP
#define INCLUDED_TEST_GRAPH_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/sink_node_ctrl.hpp>
#include <uhd/rfnoc/source_node_ctrl.hpp>

#define MAKE_NODE(name) test_node::sptr name(new test_node(#name));

// Smallest possible test class
class test_node : virtual public uhd::rfnoc::sink_node_ctrl, virtual public uhd::rfnoc::source_node_ctrl
{
public:
    typedef boost::shared_ptr<test_node> sptr;

    test_node(const std::string &test_id) : _test_id(test_id) {};

    void issue_stream_cmd(const uhd::stream_cmd_t &, const size_t) {/* nop */};

    std::string get_test_id() const { return _test_id; };

private:
    const std::string _test_id;

}; /* class test_node */

void connect_nodes(uhd::rfnoc::source_node_ctrl::sptr A, uhd::rfnoc::sink_node_ctrl::sptr B)
{
    const size_t actual_src_port = A->connect_downstream(B);
    const size_t actual_dst_port = B->connect_upstream(A);
    A->set_downstream_port(actual_src_port, actual_dst_port);
    B->set_upstream_port(actual_dst_port, actual_src_port);
}

#endif /* INCLUDED_TEST_GRAPH_HPP */
// vim: sw=4 et:
