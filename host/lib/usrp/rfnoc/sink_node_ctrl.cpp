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

#include <uhd/usrp/rfnoc/sink_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/source_node_ctrl.hpp>

using namespace uhd::rfnoc;

void sink_node_ctrl::register_upstream_node(
        node_ctrl_base::sptr upstream_node,
        size_t port
) {
    // If no port was specified, use the first free one:
    if (port == ANY_PORT) {
        port = 0;
        while (_upstream_nodes.count(port))
            port++;
    }

    if (not boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node)) {
        throw uhd::type_error("Attempting to register a non-source block as upstream.");
    }

    _upstream_nodes[port] = boost::weak_ptr<node_ctrl_base>(upstream_node);
}

