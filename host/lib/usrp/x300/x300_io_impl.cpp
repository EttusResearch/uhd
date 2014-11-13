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

#include "x300_regs.hpp"
#include "x300_impl.hpp"
#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <uhd/transport/nirio_zero_copy.hpp>
#include "async_packet_handler.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/log.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * update streamer rates
 **********************************************************************/
// TODO: Move to device3?
void x300_impl::update_tick_rate(size_t mb_index, const double rate)
{
    BOOST_FOREACH(const std::string &block_id, _rx_streamers.keys()) {
        if (rfnoc::block_id_t(block_id).get_device_no() != mb_index) {
            continue;
        }
        UHD_MSG(status) << "setting rx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
    BOOST_FOREACH(const std::string &block_id, _tx_streamers.keys()) {
        if (rfnoc::block_id_t(block_id).get_device_no() != mb_index) {
            continue;
        }
        UHD_MSG(status) << "setting tx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
}

// TODO: Move to device3?
void x300_impl::update_rx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    // FIXME this does not take into account which device this is on
    std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _rx_streamers.has_key(radio_block_id)) return;
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[radio_block_id].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = mb.radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

// TODO: Move to device3?
void x300_impl::update_tx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    // FIXME this does not take into account which device this is on
    std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _tx_streamers.has_key(radio_block_id)) return;
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[radio_block_id].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = mb.radio_perifs[dspno].duc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

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

// vim: sw=4 expandtab:
