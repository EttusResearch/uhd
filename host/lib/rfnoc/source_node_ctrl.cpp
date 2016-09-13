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
#include <uhd/rfnoc/source_node_ctrl.hpp>
#include <uhd/rfnoc/sink_node_ctrl.hpp>

using namespace uhd::rfnoc;

size_t source_node_ctrl::connect_downstream(
        node_ctrl_base::sptr downstream_node,
        size_t port,
        const uhd::device_addr_t &args
) {
    boost::mutex::scoped_lock lock(_output_mutex);
    port = _request_output_port(port, args);
    _register_downstream_node(downstream_node, port);
    return port;
}

void source_node_ctrl::set_rx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "source_node_ctrl::set_rx_streamer() " << port << " -> " << active << std::endl;

    /* This will enable all upstream blocks:
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, list_upstream_nodes()) {
        sptr curr_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        if (curr_upstream_block_ctrl) {
            curr_upstream_block_ctrl->set_rx_streamer(
                    active,
                    get_upstream_port(upstream_node.first)
            );
        }
    }
    */

    // This only enables 1:1 (if output 1 is enabled, enable what's connected to input 1)
    if (list_upstream_nodes().count(port)) {
        source_node_ctrl::sptr this_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(list_upstream_nodes().at(port).lock());
        if (this_upstream_block_ctrl) {
            this_upstream_block_ctrl->set_rx_streamer(
                    active,
                    get_upstream_port(port)
            );
        }
    }

    _rx_streamer_active[port] = active;
}

size_t source_node_ctrl::_request_output_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    return utils::node_map_find_first_free(_downstream_nodes, suggested_port);
}

void source_node_ctrl::_register_downstream_node(
        node_ctrl_base::sptr downstream_node,
        size_t port
) {
    // Do all the checks:
    if (port == ANY_PORT) {
        throw uhd::type_error(str(
                boost::format("[%s] Invalid output port number (ANY).")
                % unique_id()
        ));
    }
    if (_downstream_nodes.count(port) and not _downstream_nodes[port].expired()) {
        throw uhd::runtime_error(str(boost::format("On node %s, output port %d is already connected.") % unique_id() % port));
    }
    if (not boost::dynamic_pointer_cast<sink_node_ctrl>(downstream_node)) {
        throw uhd::type_error("Attempting to register a non-sink block as downstream.");
    }
    // Alles klar, Herr Kommissar :)

    _downstream_nodes[port] = boost::weak_ptr<node_ctrl_base>(downstream_node);
}

