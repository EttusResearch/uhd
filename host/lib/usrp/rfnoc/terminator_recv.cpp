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

#include "terminator_recv.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/rfnoc/source_node_ctrl.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;

size_t terminator_recv::_count = 0;

terminator_recv::terminator_recv() :
    _term_index(_count),
    _samp_rate(rate_node_ctrl::RATE_UNDEFINED),
    _tick_rate(tick_node_ctrl::RATE_UNDEFINED)
{
    _count++;
}

std::string terminator_recv::unique_id() const
{
    return str(boost::format("RX Terminator %d") % _term_index);
}

void terminator_recv::set_tx_streamer(bool)
{
    /* nop */
}

void terminator_recv::set_rx_streamer(bool active)
{
    // TODO this is identical to source_node_ctrl::set_rx_streamer() -> factor out
    UHD_MSG(status) << "[" << unique_id() << "] terminator_recv::set_rx_streamer() " << active << std::endl;
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, _upstream_nodes) {
        source_node_ctrl::sptr curr_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        if (curr_upstream_block_ctrl) {
            curr_upstream_block_ctrl->set_rx_streamer(active);
        }
    }
}

terminator_recv::~terminator_recv()
{
    UHD_MSG(status) << "[" << unique_id() << "] terminator_recv::~terminator_recv() " << std::endl;
    set_rx_streamer(false);
}

