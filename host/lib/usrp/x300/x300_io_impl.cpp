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
#include "validate_subdev_spec.hpp"
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
 * Setup dboard muxing for IQ
 **********************************************************************/
void x300_impl::update_subdev_spec(const std::string &tx_rx, const size_t mb_i, const subdev_spec_t &spec)
{
    UHD_ASSERT_THROW(tx_rx == "tx" or tx_rx == "rx");
    UHD_ASSERT_THROW(mb_i < _mb.size());
    const std::string mb_name = boost::lexical_cast<std::string>(mb_i);
    fs_path mb_root = "/mboards/" + mb_name;

    //sanity checking
    validate_subdev_spec(_tree, spec, tx_rx, mb_name);
    UHD_ASSERT_THROW(spec.size() <= 2);
    if (spec.size() == 1) {
        UHD_ASSERT_THROW(spec[0].db_name == "A" || spec[0].db_name == "B");
    }
    else if (spec.size() == 2) {
        UHD_ASSERT_THROW(
            (spec[0].db_name == "A" && spec[1].db_name == "B") ||
            (spec[0].db_name == "B" && spec[1].db_name == "A")
        );
    }

    std::vector<size_t> chan_to_dsp_map(spec.size(), 0);
    // setup mux for this spec
    for (size_t i = 0; i < spec.size(); i++)
    {
        const int radio_idx = _mb[mb_i].get_radio_index(spec[i].db_name);
        chan_to_dsp_map[i] = radio_idx;

        //extract connection
        const std::string conn = _tree->access<std::string>(mb_root / "dboards" / spec[i].db_name / (tx_rx + "_frontends") / spec[i].sd_name / "connection").get();

        if (tx_rx == "tx") {
            //swap condition
            _mb[mb_i].radio_perifs[radio_idx].tx_fe->set_mux(conn);
        } else {
            //swap condition
            const bool fe_swapped = (conn == "QI" or conn == "Q");
            _mb[mb_i].radio_perifs[radio_idx].ddc->set_mux(conn, fe_swapped);
            //see usrp/io_impl.cpp if multiple DSPs share the frontend:
            _mb[mb_i].radio_perifs[radio_idx].rx_fe->set_mux(fe_swapped);
        }
    }

    _tree->access<std::vector<size_t> >(mb_root / (tx_rx + "_chan_dsp_mapping")).set(chan_to_dsp_map);
}


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

// vim: sw=4 expandtab:
