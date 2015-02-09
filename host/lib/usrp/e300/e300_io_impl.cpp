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

/***********************************************************************
 * Legacy Subdev-spec support
 **********************************************************************/
void e300_impl::subdev_to_blockid(
        const subdev_spec_pair_t &spec, const size_t mb_i,
        rfnoc::block_id_t &block_id, device_addr_t &
) {
    UHD_ASSERT_THROW(spec.db_name == "A");
    UHD_ASSERT_THROW(spec.sd_name == "A" || spec.sd_name == "B");

    block_id.set_device_no(mb_i);
    block_id.set_block_name("Radio");
    block_id.set_block_count(spec.sd_name == "A" ? 0 : 1);
}

subdev_spec_pair_t e300_impl::blockid_to_subdev(
        const rfnoc::block_id_t &block_id, const uhd::device_addr_t &
) {
    UHD_ASSERT_THROW(block_id.get_block_count() == 0 || block_id.get_block_count() == 1);
    UHD_ASSERT_THROW(block_id.get_block_name() == "Radio");

    subdev_spec_pair_t spec;
    spec.db_name = "A";
    spec.sd_name = (block_id.get_block_count() == 0) ? "A" : "B";
    return spec;
}

}}} // namespace
