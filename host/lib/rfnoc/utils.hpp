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

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <boost/lexical_cast.hpp>
#include <set>

namespace uhd { namespace rfnoc { namespace utils {

    /*! If \p suggested_port equals ANY_PORT, return the first available
     * port number on \p nodes. Otherwise, return \p suggested_port.
     *
     * If \p allowed_ports is given, another condition is that the port
     * number must be listed in here.
     * If \p allowed_ports is not specified or empty, the assumption is
     * that all ports are valid.
     *
     * On failure, ANY_PORT is returned.
     */
    static size_t node_map_find_first_free(
            node_ctrl_base::node_map_t nodes,
            const size_t suggested_port,
            const std::set<size_t> allowed_ports=std::set<size_t>()
    ) {
        size_t port = suggested_port;
        if (port == ANY_PORT) {
            if (allowed_ports.empty()) {
                port = 0;
                while (nodes.count(port) and (port != ANY_PORT)) {
                    port++;
                }
            } else {
                BOOST_FOREACH(const size_t allowed_port, allowed_ports) {
                    if (not nodes.count(port)) {
                        return allowed_port;
                    }
                    return ANY_PORT;
                }
            }
        } else {
            if (not (allowed_ports.empty() or allowed_ports.count(port))) {
                return ANY_PORT;
            }
        }
        return port;
    }

    template <typename T>
    static std::set<T> str_list_to_set(const std::vector<std::string> &list) {
        std::set<T> return_set;
        BOOST_FOREACH(const std::string &S, list) {
            return_set.insert(boost::lexical_cast<T>(S));
        }
        return return_set;
    }

}}}; /* namespace uhd::rfnoc::utils */

#endif /* INCLUDED_LIBUHD_RFNOC_UTILS_HPP */
// vim: sw=4 et:
