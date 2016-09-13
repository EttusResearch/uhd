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

#include "utils.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/rfnoc/sink_node_ctrl.hpp>
#include <uhd/rfnoc/source_node_ctrl.hpp>

using namespace uhd::rfnoc;

size_t sink_node_ctrl::connect_upstream(
        node_ctrl_base::sptr upstream_node,
        size_t port,
        const uhd::device_addr_t &args
) {
    boost::mutex::scoped_lock lock(_input_mutex);
    port = _request_input_port(port, args);
    _register_upstream_node(upstream_node, port);
    return port;
}

void sink_node_ctrl::set_tx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "sink_node_ctrl::set_tx_streamer() " << active << " " << port << std::endl;

    /* Enable all downstream connections:
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t downstream_node, list_downstream_nodes()) {
        sptr curr_downstream_block_ctrl =
            boost::dynamic_pointer_cast<sink_node_ctrl>(downstream_node.second.lock());
        if (curr_downstream_block_ctrl) {
            curr_downstream_block_ctrl->set_tx_streamer(
                    active,
                    get_downstream_port(downstream_node.first)
            );
        }
    }
    */

    // Only enable 1:1
    if (list_downstream_nodes().count(port)) {
        sink_node_ctrl::sptr this_downstream_block_ctrl =
            boost::dynamic_pointer_cast<sink_node_ctrl>(list_downstream_nodes().at(port).lock());
        if (this_downstream_block_ctrl) {
            this_downstream_block_ctrl->set_tx_streamer(
                    active,
                    get_downstream_port(port)
            );
        }
    }

    _tx_streamer_active[port] = active;
}

size_t sink_node_ctrl::_request_input_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    return utils::node_map_find_first_free(_upstream_nodes, suggested_port);
}

void sink_node_ctrl::_register_upstream_node(
        node_ctrl_base::sptr upstream_node,
        size_t port
) {
    // Do all the checks:
    if (port == ANY_PORT) {
        throw uhd::type_error("Invalid input port number.");
    }
    if (_upstream_nodes.count(port) and not _upstream_nodes[port].expired()) {
        throw uhd::runtime_error(str(boost::format("On node %s, input port %d is already connected.") % unique_id() % port));
    }
    if (not boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node)) {
        throw uhd::type_error("Attempting to register a non-source block as upstream.");
    }
    // Alles klar, Herr Kommissar :)

    _upstream_nodes[port] = boost::weak_ptr<node_ctrl_base>(upstream_node);
}
