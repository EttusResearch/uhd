//
// Copyright 2015 Per Vice Corporation
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

#ifndef INCLUDED_UHD_TRANSPORT_UDP_STREAM_HPP
#define INCLUDED_UHD_TRANSPORT_UDP_STREAM_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

class UHD_API udp_stream : boost::noncopyable{
public:
    typedef boost::shared_ptr<udp_stream> sptr;

    virtual ~udp_stream(void) = 0;

    //! The maximum number of bytes per udp packet.
    static const size_t mtu = 1500 - 20 - 8; //default ipv4 mtu - ipv4 header - udp header

    /*!
     * Make an RX Stream
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_rx_stream(const std::string &addr, const std::string &port);

    /*!
     * Make a TX Stream
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_tx_stream(const std::string &addr, const std::string &port);

    /*!
     * Stream data out
     *
     * \param buff single asio buffer
     * \return the number of bytes sent
     */
    virtual size_t stream_out(const void* buff, size_t size) = 0;

    /*!
     * Stream data in
     *
     * \param buff a mutable buffer to receive into
     * \param timeout the timeout in seconds
     * \return the number of bytes received or zero on timeout
     */
    virtual size_t stream_in(void* buff, size_t size, double timeout = 0.1) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_UDP_STREAM_HPP */
