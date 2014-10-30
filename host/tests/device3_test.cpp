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

#include <exception>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/device3.hpp>
#include <uhd/usrp/rfnoc/block_ctrl.hpp>
#include <uhd/usrp/rfnoc/null_block_ctrl.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

static const boost::uint64_t TEST_NOC_ID = 0xAAAABBBBCCCCDDDD;
static const sid_t TEST_SID0 = 0x00000200; // 0.0.2.0
static const sid_t TEST_SID1 = 0x00000201; // 0.0.2.1

// Pseudo-wb-iface
class pseudo_wb_iface_impl : public uhd::wb_iface
{
  public:
    pseudo_wb_iface_impl() {};
    ~pseudo_wb_iface_impl() {};

    void poke64(const wb_addr_type addr, const boost::uint64_t data) {
        std::cout << str(boost::format("poke64 to addr: %016X, data == %016X") % addr % data) << std::endl;
    };

    boost::uint64_t peek64(const wb_addr_type addr) {
        std::cout << str(boost::format("peek64 to addr: %016X") % addr) << std::endl;
        switch (addr) {
            case SR_READBACK_REG_ID:
                return TEST_NOC_ID;
            case SR_READBACK_REG_BUFFALLOC0:
                return 0x000000000000000B;
            case SR_READBACK_REG_BUFFALLOC1:
                return 0x0000000000000000;
            case SR_READBACK_REG_USER:
                return 0x0123456789ABCDEF;
            default:
                return 0;
        }
        return 0;
    }

    void poke32(const wb_addr_type addr, const boost::uint32_t data) {
        std::cout << str(boost::format("poke32 to addr: %08X, data == %08X") % addr % data) << std::endl;
    }

    boost::uint32_t peek32(const wb_addr_type addr) {
        std::cout << str(boost::format("peek32 to addr: %08X") % addr) << std::endl;
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
        wb_iface::sptr ctrl_iface = wb_iface::sptr(new pseudo_wb_iface_impl());

        // Add two block controls:
        uhd::rfnoc::make_args_t make_args;
        make_args.ctrl_iface = ctrl_iface;
        make_args.ctrl_sid = TEST_SID0;
        make_args.device_index = 0;
        make_args.tree = _tree;
        make_args.is_big_endian = false;
        std::cout << "Generating block controls " << std::endl;

        _rfnoc_block_ctrl.push_back( block_ctrl_base::make(make_args) );

        make_args.ctrl_sid = TEST_SID1;
        _rfnoc_block_ctrl.push_back( block_ctrl::make(make_args, 0) );
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

};

device3::sptr make_pseudo_device()
{
    return device3::sptr(new pseudo_device3_impl());
}

BOOST_AUTO_TEST_CASE(test_device3) {
    device3::sptr my_device = make_pseudo_device();

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl_base::sptr block0 = my_device->find_block_ctrl("Block");
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 = my_device->get_block_ctrl(block_id_t("0/NullSrcSink_0"));
    BOOST_REQUIRE(block1);
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/NullSrcSink_0");
}

BOOST_AUTO_TEST_CASE(test_device3_cast) {
    device3::sptr my_device = make_pseudo_device();

    std::cout << "Getting block 0..." << std::endl;
    block_ctrl::sptr block0 = my_device->find_block_ctrl<block_ctrl>("Block");
    BOOST_REQUIRE(block0);
    BOOST_CHECK_EQUAL(block0->get_block_id(), "0/Block_0");

    std::cout << "Getting block 1..." << std::endl;
    block_ctrl_base::sptr block1 = my_device->get_block_ctrl<null_block_ctrl>(block_id_t("0/NullSrcSink_0"));
    BOOST_CHECK_EQUAL(block1->get_block_id(), "0/NullSrcSink_0");
}

BOOST_AUTO_TEST_CASE(test_device3_fail) {
    device3::sptr my_device = make_pseudo_device();

    BOOST_CHECK(not my_device->find_block_ctrl("FooBarBlock"));
    BOOST_CHECK(not my_device->find_block_ctrl<block_ctrl>("FooBarBlock"));

    BOOST_REQUIRE_THROW(
            my_device->get_block_ctrl(block_id_t("0/FooBarBlock_17")),
            uhd::lookup_error
    );
}

// vim: sw=4 et:
