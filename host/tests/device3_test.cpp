//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include "mock_ctrl_iface_impl.hpp"
#include "mock_zero_copy.hpp"
#include <uhd/device3.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/block_ctrl.hpp>
#include <uhd/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/graph_impl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <exception>
#include <iostream>


using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::transport::vrt;
using uhd::transport::managed_recv_buffer;
using uhd::transport::managed_send_buffer;

static const sid_t TEST_SID0 = 0x00000200; // 0.0.2.0
static const sid_t TEST_SID1 = 0x00000210; // 0.0.2.F

uhd::both_xports_t make_mock_transport(const uhd::sid_t& tx_sid)
{
    uhd::both_xports_t xports;
    xports.send_sid = tx_sid;
    xports.recv_sid = tx_sid.reversed();
    xports.send = boost::make_shared<mock_zero_copy>(if_packet_info_t::LINK_TYPE_CHDR);
    xports.recv = xports.send;
    xports.send_buff_size = xports.send->get_send_frame_size();
    xports.recv_buff_size = xports.recv->get_recv_frame_size();
    return xports;
}

// Mock-device
class mock_device3_impl : public uhd::device3,
                          public boost::enable_shared_from_this<mock_device3_impl>
{
public:
    mock_device3_impl()
    {
        _tree = uhd::property_tree::make();
        _tree->create<std::string>("/name").set("Test Mock-Device3");
        // We can re-use this:
        std::map<size_t, ctrl_iface::sptr> ctrl_ifaces{
            {0, ctrl_iface::sptr(new mock_ctrl_iface_impl())}};

        // Add two block controls:
        uhd::rfnoc::make_args_t make_args;
        make_args.ctrl_ifaces  = ctrl_ifaces;
        make_args.base_address = TEST_SID0.get_dst();
        make_args.device_index = 0;
        make_args.tree         = _tree;
        std::cout << "[MOCK] Generating block controls 1/2:" << std::endl;
        _rfnoc_block_ctrl.push_back(block_ctrl_base::make(make_args));
        std::cout << "[MOCK] Generating block controls 2/2:" << std::endl;
        make_args.base_address = TEST_SID1.get_dst();
        _rfnoc_block_ctrl.push_back(block_ctrl::make(make_args));
    }

    rx_streamer::sptr get_rx_stream(const stream_args_t& args)
    {
        throw uhd::not_implemented_error(args.args.to_string());
    }

    tx_streamer::sptr get_tx_stream(const stream_args_t& args)
    {
        throw uhd::not_implemented_error(args.args.to_string());
    }

    bool recv_async_msg(async_metadata_t& async_metadata, double timeout)
    {
        throw uhd::not_implemented_error(
            str(boost::format("%d %f") % async_metadata.channel % timeout));
    }

    rfnoc::graph::sptr create_graph(const std::string& name)
    {
        sid_t async_sid(0);
        async_sid.set_dst_addr(2);
        auto async_xports = make_mock_transport(async_sid);

        auto async_msg_handler = uhd::rfnoc::async_msg_handler::make(async_xports.recv,
            async_xports.send,
            async_xports.send_sid,
            async_xports.endianness);
        auto graph             = boost::make_shared<uhd::rfnoc::graph_impl>(
            name, shared_from_this(), async_msg_handler);
        return graph;
    }
};

device3::sptr make_mock_device()
{
    return device3::sptr(new mock_device3_impl());
}

class mock_block_ctrl : public block_ctrl
{
    int foo;
};

BOOST_AUTO_TEST_CASE(test_device3)
{
    device3::sptr my_device = make_mock_device();

    std::cout << "Checking block 0..." << std::endl;
    BOOST_REQUIRE(my_device->find_blocks("Block").size());

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl_base::sptr block0 =
        my_device->get_block_ctrl(my_device->find_blocks("Block")[0]);
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Checking block 1..." << std::endl;
    BOOST_REQUIRE(my_device->has_block(block_id_t("0/Block_1")));

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 = my_device->get_block_ctrl(block_id_t("0/Block_1"));
    BOOST_REQUIRE(block1);
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/Block_1");
}


BOOST_AUTO_TEST_CASE(test_device3_graph)
{
    auto my_device = make_mock_device();
    std::cout << "Start device3 test graph.." << std::endl;
    std::cout << "Checking block 0..." << std::endl;
    BOOST_REQUIRE(my_device->find_blocks("Block").size());
    std::cout << "Getting block 0..." << std::endl;
    auto block0 = my_device->get_block_ctrl(my_device->find_blocks("Block")[0]);
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Checking block 1..." << std::endl;
    BOOST_REQUIRE(my_device->has_block(block_id_t("0/Block_1")));

    std::cout << "Getting block 1..." << std::endl;
    auto block1 = my_device->get_block_ctrl(block_id_t("0/Block_1"));
    BOOST_REQUIRE(block1);
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/Block_1");
    std::cout << "Creating graph..." << std::endl;
    auto graph = my_device->create_graph("test_graph");
    BOOST_CHECK(graph);
    std::cout << "Connecting block_0 to block_1 ..." << std::endl;
    graph->connect(block_id_t("0/Block_0"), 0, block_id_t("0/Block_1"), 0);

    BOOST_CHECK_EQUAL(block0->list_upstream_nodes().size(), 0);
    BOOST_CHECK_EQUAL(block0->list_downstream_nodes().size(), 1);
    BOOST_CHECK_EQUAL(
        block0->list_downstream_nodes()[0].lock()->unique_id(), "0/Block_1");
    BOOST_CHECK_EQUAL(block1->list_upstream_nodes().size(), 1);
    BOOST_CHECK_EQUAL(block1->list_downstream_nodes().size(), 0);
    BOOST_CHECK_EQUAL(block1->list_upstream_nodes()[0].lock()->unique_id(), "0/Block_0");
}

BOOST_AUTO_TEST_CASE(test_device3_cast)
{
    device3::sptr my_device = make_mock_device();

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl::sptr block0 =
        my_device->get_block_ctrl<block_ctrl>(block_id_t("0/Block_0"));
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 =
        my_device->get_block_ctrl<block_ctrl>(block_id_t("0/Block_1"));
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/Block_1");
}

BOOST_AUTO_TEST_CASE(test_device3_fail)
{
    device3::sptr my_device = make_mock_device();

    BOOST_CHECK(not my_device->has_block(block_id_t("0/FooBarBlock_0")));
    BOOST_CHECK(not my_device->has_block<mock_block_ctrl>(block_id_t("0/Block_1")));

    BOOST_CHECK(my_device->find_blocks("FooBarBlock").size() == 0);
    BOOST_CHECK(my_device->find_blocks<block_ctrl>("FooBarBlock").size() == 0);

    BOOST_REQUIRE_THROW(
        my_device->get_block_ctrl(block_id_t("0/FooBarBlock_17")), uhd::lookup_error);
    BOOST_REQUIRE_THROW(
        my_device->get_block_ctrl<mock_block_ctrl>(block_id_t("0/Block_1")),
        uhd::lookup_error);
}

// vim: sw=4 et:
