//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/transport/if_addrs.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/cstdint.hpp>
#include <iostream>

/***********************************************************************
 * Interface address discovery through ifaddrs api
 **********************************************************************/
#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>

static boost::asio::ip::address_v4 sockaddr_to_ip_addr(sockaddr *addr){
    return boost::asio::ip::address_v4(ntohl(
        reinterpret_cast<sockaddr_in*>(addr)->sin_addr.s_addr
    ));
}

std::vector<uhd::transport::if_addrs_t> uhd::transport::get_if_addrs(void){
    std::vector<if_addrs_t> if_addrs;
    struct ifaddrs *ifap;
    if (getifaddrs(&ifap) == 0){
        for (struct ifaddrs *iter = ifap; iter != NULL; iter = iter->ifa_next){
            //ensure that the entries are valid
            if (iter->ifa_addr == NULL) continue;
            if (iter->ifa_addr->sa_family != AF_INET) continue;
            if (iter->ifa_netmask->sa_family != AF_INET) continue;
            if (iter->ifa_broadaddr->sa_family != AF_INET) continue;

            //append a new set of interface addresses
            if_addrs_t if_addr;
            if_addr.inet = sockaddr_to_ip_addr(iter->ifa_addr).to_string();
            if_addr.mask = sockaddr_to_ip_addr(iter->ifa_netmask).to_string();
            if_addr.bcast = sockaddr_to_ip_addr(iter->ifa_broadaddr).to_string();

            //correct the bcast address when its same as the gateway
            if (if_addr.inet == if_addr.bcast or sockaddr_to_ip_addr(iter->ifa_broadaddr) == boost::asio::ip::address_v4(0)){
                //manually calculate broadcast address
                //https://svn.boost.org/trac/boost/ticket/5198
                const boost::uint32_t addr = sockaddr_to_ip_addr(iter->ifa_addr).to_ulong();
                const boost::uint32_t mask = sockaddr_to_ip_addr(iter->ifa_netmask).to_ulong();
                const boost::uint32_t bcast = (addr & mask) | ~mask;
                if_addr.bcast = boost::asio::ip::address_v4(bcast).to_string();
            }

            if_addrs.push_back(if_addr);
        }
        freeifaddrs(ifap);
    }
    return if_addrs;
}

#endif /* HAVE_GETIFADDRS */

/***********************************************************************
 * Interface address discovery through windows api
 **********************************************************************/
#ifdef HAVE_SIO_GET_INTERFACE_LIST
#include <winsock2.h>

std::vector<uhd::transport::if_addrs_t> uhd::transport::get_if_addrs(void){
    std::vector<if_addrs_t> if_addrs;
    SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
    if (sd == SOCKET_ERROR) {
        std::cerr << "Failed to get a socket. Error " << WSAGetLastError() <<
            std::endl; return if_addrs;
    }

    INTERFACE_INFO InterfaceList[20];
    unsigned long nBytesReturned;
    if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
			sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) {
        std::cerr << "Failed calling WSAIoctl: error " << WSAGetLastError() <<
				std::endl;
		return if_addrs;
    }

    int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);
    for (int i = 0; i < nNumInterfaces; ++i) {
        boost::uint32_t iiAddress = ntohl(reinterpret_cast<sockaddr_in&>(InterfaceList[i].iiAddress).sin_addr.s_addr);
        boost::uint32_t iiNetmask = ntohl(reinterpret_cast<sockaddr_in&>(InterfaceList[i].iiNetmask).sin_addr.s_addr);
        boost::uint32_t iiBroadcastAddress = (iiAddress & iiNetmask) | ~iiNetmask;

        if_addrs_t if_addr;
        if_addr.inet = boost::asio::ip::address_v4(iiAddress).to_string();
        if_addr.mask = boost::asio::ip::address_v4(iiNetmask).to_string();
        if_addr.bcast = boost::asio::ip::address_v4(iiBroadcastAddress).to_string();
        if_addrs.push_back(if_addr);
    }

    return if_addrs;
}

#endif /* HAVE_SIO_GET_INTERFACE_LIST */

/***********************************************************************
 * Interface address discovery not included
 **********************************************************************/
#ifdef HAVE_IF_ADDRS_DUMMY

std::vector<uhd::transport::if_addrs_t> uhd::transport::get_if_addrs(void){
    return std::vector<if_addrs_t>();
}

#endif /* HAVE_IF_ADDRS_DUMMY */
