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

#include <uhd/usrp/rfnoc/node_ctrl_base.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

void node_ctrl_base::set_args(const uhd::device_addr_t &args)
{
    UHD_MSG(status) << "node_ctrl_base::set_args() " << args.to_string() << std::endl;
    _args = args;
    _post_args_hook();
}

void node_ctrl_base::clear()
{
    UHD_MSG(status) << "node_ctrl_base::clear() " << std::endl;
    // Reset connections:
    _upstream_nodes.clear();
    _downstream_nodes.clear();
}

void node_ctrl_base::register_upstream_node(
        node_ctrl_base::sptr upstream_node,
        size_t port
) {
    // If no port was specified, use the first free one:
    if (port == ANY_PORT) {
        port = 0;
        while (_downstream_nodes.count(port))
            port++;
    }

    _upstream_nodes[port] = boost::weak_ptr<node_ctrl_base>(upstream_node);
}

void node_ctrl_base::register_downstream_node(
        node_ctrl_base::sptr downstream_node,
        size_t port
) {
    // If no port was specified, use the first free one:
    if (port == ANY_PORT) {
        port = 0;
        while (_downstream_nodes.count(port))
            port++;
    }

    _downstream_nodes[port] = boost::weak_ptr<node_ctrl_base>(downstream_node);
}

