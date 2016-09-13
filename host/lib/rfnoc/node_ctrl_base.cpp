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

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

std::string node_ctrl_base::unique_id() const
{
    // Most instantiations will override this, so we don't need anything
    // more elegant here.
    return str(boost::format("%08X") % size_t(this));
}

void node_ctrl_base::clear()
{
    UHD_RFNOC_BLOCK_TRACE() << "node_ctrl_base::clear() " << std::endl;
    // Reset connections:
    _upstream_nodes.clear();
    _downstream_nodes.clear();
}

void node_ctrl_base::_register_downstream_node(
    node_ctrl_base::sptr,
    size_t
) {
    throw uhd::runtime_error("Attempting to register a downstream block on a non-source node.");
}

void node_ctrl_base::_register_upstream_node(
    node_ctrl_base::sptr,
    size_t
) {
    throw uhd::runtime_error("Attempting to register an upstream block on a non-sink node.");
}

void node_ctrl_base::set_downstream_port(
        const size_t this_port,
        const size_t remote_port
) {
    if (not _downstream_nodes.count(this_port) and remote_port != ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot set remote downstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    _downstream_ports[this_port] = remote_port;
}

size_t node_ctrl_base::get_downstream_port(const size_t this_port)
{
    if (not _downstream_ports.count(this_port)
        or not _downstream_nodes.count(this_port)
        or _downstream_ports[this_port] == ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot retrieve remote downstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    return _downstream_ports[this_port];
}

void node_ctrl_base::set_upstream_port(
        const size_t this_port,
        const size_t remote_port
) {
    if (not _upstream_nodes.count(this_port) and remote_port != ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot set remote upstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    _upstream_ports[this_port] = remote_port;
}

size_t node_ctrl_base::get_upstream_port(const size_t this_port)
{
    if (not _upstream_ports.count(this_port)
        or not _upstream_nodes.count(this_port)
        or _upstream_ports[this_port] == ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot retrieve remote upstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    return _upstream_ports[this_port];
}

