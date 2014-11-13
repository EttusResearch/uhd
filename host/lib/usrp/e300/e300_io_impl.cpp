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

#include "e300_regs.hpp"
#include "e300_impl.hpp"
#include "e300_fpga_defs.hpp"
#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <boost/bind.hpp>
#include <uhd/utils/tasks.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

namespace uhd { namespace usrp { namespace e300 {

/***********************************************************************
 * update streamer rates
 **********************************************************************/
void e300_impl::_check_tick_rate_with_current_streamers(const double rate)
{
    bool enb_tx1 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_1/tx_active")) {
        enb_tx1 = _tree->access<bool>("/mboards/0/xbar/Radio_1/tx_active").get();
    }

    bool enb_tx2 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_0/tx_active")) {
        enb_tx2 = _tree->access<bool>("/mboards/0/xbar/Radio_0/tx_active").get();
    }

    bool enb_rx1 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_1/rx_active")) {
        enb_rx1 = _tree->access<bool>("/mboards/0/xbar/Radio_1/rx_active").get();
    }

    bool enb_rx2 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_0/tx_active")) {
        enb_rx2 = _tree->access<bool>("/mboards/0/xbar/Radio_0/rx_active").get();
    }

    const size_t max_tx_chan_count = (enb_tx1 ? 1 : 0) + (enb_tx2 ? 1 : 0);
    const size_t max_rx_chan_count = (enb_rx1 ? 1 : 0) + (enb_rx2 ? 1 : 0);
    _enforce_tick_rate_limits(max_rx_chan_count, rate, "RX");
    _enforce_tick_rate_limits(max_tx_chan_count, rate, "TX");
}

void e300_impl::_update_tick_rate(const double rate)
{
    _check_tick_rate_with_current_streamers(rate);

    BOOST_FOREACH(const std::string &block_id, _rx_streamers.keys()) {
        UHD_MSG(status) << "setting rx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
    BOOST_FOREACH(const std::string &block_id, _tx_streamers.keys()) {
        UHD_MSG(status) << "setting tx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
}

void e300_impl::_update_rx_samp_rate(const size_t dspno, const double rate)
{
    const std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _rx_streamers.has_key(radio_block_id))
        return;
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[radio_block_id].lock());
    if (not my_streamer)
        return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = _radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void e300_impl::_update_tx_samp_rate(const size_t dspno, const double rate)
{
    const std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _tx_streamers.has_key(radio_block_id))
        return;
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[radio_block_id].lock());
    if (not my_streamer)
        return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = _radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

/***********************************************************************
 * frontend selection
 **********************************************************************/
void e300_impl::_update_subdev_spec(
        const std::string &txrx,
        const uhd::usrp::subdev_spec_t &spec)
{
    //sanity checking
    if (spec.size())
        validate_subdev_spec(_tree, spec, "rx");

    UHD_ASSERT_THROW(spec.size() <= fpga::NUM_RADIOS);

    if (spec.size() >= 1)
    {
        UHD_ASSERT_THROW(spec[0].db_name == "A");
        UHD_ASSERT_THROW(spec[0].sd_name == "A" or spec[0].sd_name == "B");
    }
    if (spec.size() == 2)
    {
        UHD_ASSERT_THROW(spec[1].db_name == "A");
        UHD_ASSERT_THROW(
            (spec[0].sd_name == "A" and spec[1].sd_name == "B") or
            (spec[0].sd_name == "B" and spec[1].sd_name == "A")
        );
    }

    std::vector<size_t> chan_to_dsp_map(spec.size(), 0);
    for (size_t i = 0; i < spec.size(); i++)
        chan_to_dsp_map[i] = (spec[i].sd_name == "A") ? 0 : 1;
    _tree->access<std::vector<size_t> >("/mboards/0" / (txrx + "_chan_dsp_mapping")).set(chan_to_dsp_map);

    const fs_path mb_path = "/mboards/0";

    if (txrx == "tx") {
        for (size_t i = 0; i < spec.size(); i++)
        {
            const std::string conn = _tree->access<std::string>(
                mb_path / "dboards" / spec[i].db_name /
                ("tx_frontends") / spec[i].sd_name / "connection").get();
            _radio_perifs[i].tx_fe->set_mux(conn);
        }

    } else {
        for (size_t i = 0; i < spec.size(); i++)
        {
            const std::string conn = _tree->access<std::string>(
                mb_path / "dboards" / spec[i].db_name /
                ("rx_frontends") / spec[i].sd_name / "connection").get();

            const bool fe_swapped = (conn == "QI" or conn == "Q");
            _radio_perifs[i].ddc->set_mux(conn, fe_swapped);
            _radio_perifs[i].rx_fe->set_mux(fe_swapped);
        }
    }

    this->_update_enables();
}

}}} // namespace
