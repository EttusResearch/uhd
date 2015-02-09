//
// Copyright 2013-2014 Ettus Research LLC
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

#define DEVICE3_STREAMER

#include "x300_regs.hpp"
#include "x300_impl.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "../rfnoc/radio_ctrl.hpp"
#include <uhd/transport/nirio_zero_copy.hpp>
#include "async_packet_handler.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * Hooks for get_tx_stream() and get_rx_stream()
 **********************************************************************/
device_addr_t x300_impl::get_rx_hints(size_t mb_index)
{
    device_addr_t rx_hints = _mb[mb_index].recv_args;
    // (default to a large recv buff)
    if (not rx_hints.has_key("recv_buff_size"))
    {
        if (_mb[mb_index].xport_path != "nirio") {
            //For the ethernet transport, the buffer has to be set before creating
            //the transport because it is independent of the frame size and # frames
            //For nirio, the buffer size is not configurable by the user
            #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
                //limit buffer resize on macos or it will error
                rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(X300_RX_SW_BUFF_SIZE_ETH_MACOS);
            #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
                //set to half-a-second of buffering at max rate
                rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(X300_RX_SW_BUFF_SIZE_ETH);
            #endif
        }
    }
    return rx_hints;
}


device_addr_t x300_impl::get_tx_hints(size_t mb_index)
{
    return _mb[mb_index].send_args;
}

void x300_impl::post_streamer_hooks(bool is_tx)
{
    if (not is_tx) {
        return;
    }

    // Loop through all tx streamers. Find all radios connected to one
    // streamer. Sync those.
    BOOST_FOREACH(const boost::weak_ptr<uhd::tx_streamer> &streamer_w, _tx_streamers.vals()) {
        const boost::shared_ptr<sph::send_packet_streamer> streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(streamer_w.lock());
        if (not streamer) {
            continue;
        }

        std::vector<radio_perifs_t*> radios;
        std::vector<rfnoc::radio_ctrl::sptr> radio_ctrl_blks =
            streamer->get_terminator()->find_downstream_node<rfnoc::radio_ctrl>();
        BOOST_FOREACH(const rfnoc::radio_ctrl::sptr &radio_blk, radio_ctrl_blks) {
            radio_perifs_t &perif = _mb[radio_blk->get_block_id().get_device_no()].radio_perifs[radio_blk->get_block_id().get_block_count()];
            radios.push_back(&perif);
        }
        try {
            UHD_MSG(status) << "[X300] syncing " << radios.size() << " radios " << std::endl;
            synchronize_dacs(radios);
        }
        catch(const uhd::io_error &ex) {
            throw uhd::io_error(str(boost::format("Failed to sync DACs! %s ") % ex.what()));
        }
    }
}

void x300_impl::subdev_to_blockid(
        const subdev_spec_pair_t &spec, const size_t mb_i,
        rfnoc::block_id_t &block_id, device_addr_t &block_args
) {
    UHD_ASSERT_THROW(spec.db_name == "A" || spec.db_name == "B");

    block_id.set_device_no(mb_i);
    block_id.set_block_name("Radio");
    block_id.set_block_count(spec.db_name == "A" ? 0 : 1);
    block_args["frontend"] = spec.sd_name;
}

subdev_spec_pair_t x300_impl::blockid_to_subdev(
        const rfnoc::block_id_t &block_id, const uhd::device_addr_t &block_args
) {
    UHD_ASSERT_THROW(block_id.get_block_count() == 0 || block_id.get_block_count() == 1);
    UHD_ASSERT_THROW(block_id.get_block_name() == "Radio");

    subdev_spec_pair_t spec;
    spec.db_name = (block_id.get_block_count() == 0) ? "A" : "B";
    if (block_args.has_key("frontend")) {
        spec.sd_name = block_args["frontend"];
    } else {
        fs_path db_root = fs_path("/mboards") / block_id.get_device_no() / "dboards";
        spec.sd_name = _tree->list(db_root / spec.db_name / "tx_frontends").at(0);
    }
    return spec;
}

// vim: sw=4 expandtab:
