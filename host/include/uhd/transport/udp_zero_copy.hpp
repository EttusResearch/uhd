//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TRANSPORT_UDP_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_UDP_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

/*!
 * A zero copy udp transport provides an efficient way to handle data.
 * by avoiding the extra copy when recv() or send() is called on the socket.
 * Rather, the zero copy transport gives the caller memory references.
 * The caller informs the transport when it is finished with the reference.
 *
 * On linux systems, the zero copy transport can use a kernel packet ring.
 * If no platform specific solution is available, make returns a boost asio
 * implementation that wraps the functionality around a standard send/recv calls.
 */
class UHD_API udp_zero_copy : public virtual zero_copy_if{
public:
    typedef boost::shared_ptr<udp_zero_copy> sptr;

    /*!
     * Make a new zero copy udp transport:
     * This transport is for sending and receiving
     * between this host and a single endpoint.
     * The primary usage for this transport will be data transactions.
     * The underlying implementation is fast and platform specific.
     *
     * The address will be resolved, it can be a host name or ipv4.
     * The port will be resolved, it can be a port type or number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     * \param hints optional parameters to pass to the underlying transport
     */
    static sptr make(
        const std::string &addr,
        const std::string &port,
        const device_addr_t &hints = device_addr_t()
    );
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_UDP_ZERO_COPY_HPP */
