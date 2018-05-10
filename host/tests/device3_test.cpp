//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/property_tree.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/block_ctrl.hpp>
#include <uhd/rfnoc/graph.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhdlib/rfnoc/ctrl_iface.hpp>
#include <boost/test/unit_test.hpp>
#include <exception>
#include <iostream>

using namespace uhd;
using namespace uhd::rfnoc;

static const uint64_t TEST_NOC_ID = 0xAAAABBBBCCCCDDDD;
static const sid_t TEST_SID0 = 0x00000200; // 0.0.2.0
static const sid_t TEST_SID1 = 0x00000210; // 0.0.2.F

// Pseudo-wb-iface
class pseudo_ctrl_iface_impl : public ctrl_iface
{
  public:
    pseudo_ctrl_iface_impl() {}
    virtual ~pseudo_ctrl_iface_impl() {}

    uint64_t send_cmd_pkt(
            const size_t addr,
            const size_t data,
            const bool readback=false,
            const uint64_t timestamp=0
    ) {
        if (not readback) {
            std::cout << str(boost::format("[PSEUDO] poke to addr: %016X, data == %016X") % addr % data) << std::endl;
        } else {
            std::cout << str(boost::format("[PSEUDO] peek64 to addr: %016X") % data) << std::endl;
            switch (data) {
                case SR_READBACK_REG_ID:
                    return TEST_NOC_ID;
                case SR_READBACK_REG_FIFOSIZE:
                    return 0x000000000000000B;
                case SR_READBACK_REG_USER:
                    return 0x0123456789ABCDEF;
                case SR_READBACK_COMPAT:
                    return uhd::rfnoc::NOC_SHELL_COMPAT_MAJOR << 32 |
                           uhd::rfnoc::NOC_SHELL_COMPAT_MINOR;
                default:
                    return 0;
            }
        }
        return 0;
    }
};

// Pseudo-device
class pseudo_device3_impl : public uhd::device3
{
  public:
    pseudo_device3_impl()
    {
        _tree = uhd::property_tree::make();
        _tree->create<std::string>("/name").set("Test Pseudo-Device3");

        // We can re-use this:
        std::map<size_t, ctrl_iface::sptr> ctrl_ifaces{
            {0, ctrl_iface::sptr(new pseudo_ctrl_iface_impl())}
        };

        // Add two block controls:
        uhd::rfnoc::make_args_t make_args;
        make_args.ctrl_ifaces = ctrl_ifaces;
        make_args.base_address = TEST_SID0.get_dst();
        make_args.device_index = 0;
        make_args.tree = _tree;
        std::cout << "[PSEUDO] Generating block controls 1/2:" << std::endl;
        _rfnoc_block_ctrl.push_back( block_ctrl_base::make(make_args) );

        std::cout << "[PSEUDO] Generating block controls 2/2:" << std::endl;
        make_args.base_address = TEST_SID1.get_dst();
        _rfnoc_block_ctrl.push_back( block_ctrl::make(make_args) );
    }

    rx_streamer::sptr get_rx_stream(const stream_args_t &args) {
        throw uhd::not_implemented_error(args.args.to_string());
    }

    tx_streamer::sptr get_tx_stream(const stream_args_t &args) {
        throw uhd::not_implemented_error(args.args.to_string());
    }

    bool recv_async_msg(async_metadata_t &async_metadata, double timeout) {
        throw uhd::not_implemented_error(str(boost::format("%d %f") % async_metadata.channel % timeout));
    }

    rfnoc::graph::sptr create_graph(const std::string &) { return rfnoc::graph::sptr(); }
};

device3::sptr make_pseudo_device()
{
    return device3::sptr(new pseudo_device3_impl());
}

class dummy_block_ctrl : public block_ctrl {
    int foo;
};

BOOST_AUTO_TEST_CASE(test_device3) {
    device3::sptr my_device = make_pseudo_device();

    std::cout << "Checking block 0..." << std::endl;
    BOOST_REQUIRE(my_device->find_blocks("Block").size());

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl_base::sptr block0 = my_device->get_block_ctrl(my_device->find_blocks("Block")[0]);
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Checking block 1..." << std::endl;
    BOOST_REQUIRE(my_device->has_block(block_id_t("0/Block_1")));

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 = my_device->get_block_ctrl(block_id_t("0/Block_1"));
    BOOST_REQUIRE(block1);
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/Block_1");
}

BOOST_AUTO_TEST_CASE(test_device3_cast) {
    device3::sptr my_device = make_pseudo_device();

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl::sptr block0 = my_device->get_block_ctrl<block_ctrl>(block_id_t("0/Block_0"));
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 = my_device->get_block_ctrl<block_ctrl>(block_id_t("0/Block_1"));
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/Block_1");
}

BOOST_AUTO_TEST_CASE(test_device3_fail) {
    device3::sptr my_device = make_pseudo_device();

    BOOST_CHECK(not my_device->has_block(block_id_t("0/FooBarBlock_0")));
    BOOST_CHECK(not my_device->has_block<dummy_block_ctrl>(block_id_t("0/Block_1")));

    BOOST_CHECK(my_device->find_blocks("FooBarBlock").size() == 0);
    BOOST_CHECK(my_device->find_blocks<block_ctrl>("FooBarBlock").size() == 0);

    BOOST_REQUIRE_THROW(
        my_device->get_block_ctrl(block_id_t("0/FooBarBlock_17")),
        uhd::lookup_error
    );
    BOOST_REQUIRE_THROW(
        my_device->get_block_ctrl<dummy_block_ctrl>(block_id_t("0/Block_1")),
        uhd::lookup_error
    );
}

// vim: sw=4 et:
