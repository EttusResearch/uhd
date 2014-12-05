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

#ifndef INCLUDED_LIBUHD_RFNOC_UTILS_HPP
#define INCLUDED_LIBUHD_RFNOC_UTILS_HPP

#include <uhd/usrp/rfnoc/node_ctrl_base.hpp>

namespace uhd { namespace rfnoc { namespace utils {

    /*! If \p suggested_port equals ANY_PORT, return the first available
     * port number on \p nodes. Otherwise, return \p suggested_port.
     */
    static size_t node_map_find_first_free(
            node_ctrl_base::node_map_t nodes,
            const size_t suggested_port
    ) {
        size_t port = suggested_port;
        if (port == ANY_PORT) {
            port = 0;
            while (nodes.count(port) and (port != ANY_PORT)) {
                port++;
            }
        }
        return port;
    }

}}}; /* namespace uhd::rfnoc::utils */

#endif /* INCLUDED_LIBUHD_RFNOC_UTILS_HPP */
// vim: sw=4 et:
