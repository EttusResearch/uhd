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

#include <mpm/net_helper.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdint>

namespace mpm {
namespace network{

template <typename ArrayType>
std::string bytearray_to_string(const ArrayType array[], size_t elements) {
    std::stringstream result;
    for (size_t i = 0; i < elements; i++) {
        result << std::uppercase << std::setfill('0')
               << std::setw(sizeof(array[i]) *
                            2) // always produce 2 hex values for each byte
               << std::hex << +array[i]; // Implicit integer promotion
    }
    return result.str();
}

void print_net_ifaces(net_ifaces my_ifaces) {
    /* take in a net_ifaces and pretty print information
       about all detected network interfaces */
    for (const auto& iface : my_ifaces) {
        std::cout << "interface: " << iface.first << std::endl;
        std::cout << "\tMAC: " << iface.second.mac_addr << std::endl;
        for (const auto& addr : iface.second.ip_addr) {
            std::cout << "\tip address: " << addr << std::endl;
        }
    }
}

net_ifaces get_net_map() {
    /* Get a map containing a string and a net_iface struct
       to describe all adresses assigned to a interface */
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    net_ifaces net_map;

    if (getifaddrs(&ifaddr) == -1) {
        throw uhd::system_error(str(boost::format("Error: %s") % strerror(errno)));
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        /* Put the interaface name into the map, if it already exists
           we get an iterator to the existing element */
        auto result = net_map.emplace(
            std::make_pair(std::string(ifa->ifa_name), net_iface()));
        auto current_iface = result.first;

        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in)
                                                : sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return net_map;
            }
            current_iface->second.ip_addr.push_back(std::string(host));
        } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
            struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;
            uint8_t mac_addr[6];
            memcpy(&mac_addr, s->sll_addr, 6);
            current_iface->second.mac_addr = bytearray_to_string(mac_addr, 6);
        }
    }
    freeifaddrs(ifaddr);
    return net_map;
}

std::vector<std::string> get_if_addrs(const std::string& mac_addr) {
    /* Convenience wrapper to return all adresses associated with one
       mac address */
    net_ifaces my_map = get_net_map();
    for (const auto& iface : my_map) { // find
        if (iface.second.mac_addr == mac_addr) {
            return iface.second.ip_addr;
        }
    }
    return std::vector<std::string>();
}
}
}
