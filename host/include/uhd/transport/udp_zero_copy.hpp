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
#include <uhd/transport/smart_buffer.hpp>
#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

/*!
 * A zero copy udp transport provides an efficient way to handle data.
 * by avoiding the extra copy when recv() is called on the socket.
 * Rather, the zero copy transport gives the caller a memory reference.
 * The caller informs the transport when it is finished with the reference.
 *
 * On linux systems, the zero copy transport can use a kernel packet ring.
 * If no platform specific solution is available, make returns a boost asio
 * implementation that wraps the functionality around a standard recv() call.
 */
class UHD_API udp_zero_copy : boost::noncopyable{
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
     */
    static sptr make(const std::string &addr, const std::string &port);

    /*!
     * Send a single buffer.
     * Blocks until the data is sent.
     * \param buff single asio buffer
     * \return the number of bytes sent
     */
    virtual size_t send(const boost::asio::const_buffer &buff) = 0;

    /*!
     * Receive a buffer.
     * Blocks until data is received or a timeout occurs.
     * The memory is managed by the implementation.
     * \return a smart buffer (empty on timeout)
     */
    virtual smart_buffer::sptr recv(void) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_UDP_ZERO_COPY_HPP */
