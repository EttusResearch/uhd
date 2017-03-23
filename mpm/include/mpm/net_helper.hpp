//
// Copyright 2017 Ettus Research (National Instruments)
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

#include <map>
#include <vector>
#include <string>

namespace mpm {
namespace network {

/*!
 * A struct describing a single network interface
 */
using net_iface = struct net_iface {
    /*! MAC address of the interface in the form AABBCCDDEEFF */
    std::string mac_addr;
    /*! vector of associated IP addresses, contains both IPv4 and IPv6 */
    std::vector<std::string> ip_addr;
};

/*!
 * net_ifaces contains a <interfaces name, net_iface> pair
 * describing mac address and associated ip addresses for
 * each interface
 */
using net_ifaces = std::map<std::string, net_iface>;

/*!
 * Convenience function to get all ip addresses of one MAC address
 * \param MAC address in the form AABBCCDDEEFF
 * \return vector of strings containing all IP addresses with this MAC address
 */
std::vector<std::string> get_if_addrs(const std::string& mac_addr);

/*!
 * Get information about all interfaces on this system
 * \return a map with interface names as keys and the interfaces information as value
 */
net_ifaces get_net_map();

/*!
 * Pretty print net_ifaces in the style of `ip addr`
 * \param interface map net_ifaces to print
 */
void print_net_ifaces(net_ifaces my_ifaces);
}
}

#ifdef LIBMPM_PYTHON
void export_net_iface(){
    LIBMPM_BOOST_PREAMBLE("network")
        bp::def("get_if_addrs", &mpm::network::get_if_addrs);
}
#endif

