//
// Copyright 2014-2015 Ettus Research LLC
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

#include "terminator_send.hpp"
#include <boost/format.hpp>
#include <uhd/usrp/rfnoc/sink_node_ctrl.hpp>

using namespace uhd::rfnoc;

size_t terminator_send::_count = 0;

terminator_send::terminator_send() :
    _term_index(_count),
    _samp_rate(rate_node_ctrl::RATE_UNDEFINED),
    _tick_rate(tick_node_ctrl::RATE_UNDEFINED)
{
    _count++;
}

std::string terminator_send::unique_id() const
{
    return str(boost::format("TX Terminator %d") % _term_index);
}

void terminator_send::set_rx_streamer(bool)
{
    /* nop */
}

void terminator_send::set_tx_streamer(bool active)
{
    // TODO this is identical to sink_node_ctrl::set_tx_streamer() -> factor out
    UHD_MSG(status) << "[" << unique_id() << "] terminator_send::set_tx_streamer() " << active << std::endl;
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t downstream_node, _downstream_nodes) {
        sink_node_ctrl::sptr curr_downstream_block_ctrl =
            boost::dynamic_pointer_cast<sink_node_ctrl>(downstream_node.second.lock());
        if (curr_downstream_block_ctrl) {
            curr_downstream_block_ctrl->set_tx_streamer(active);
        }
    }
}

terminator_send::~terminator_send()
{
    UHD_MSG(status) << "[" << unique_id() << "] terminator_send::~terminator_send() " << std::endl;
    set_tx_streamer(false);
}

