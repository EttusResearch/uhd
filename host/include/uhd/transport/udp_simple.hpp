//
// Copyright 2010,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_UDP_SIMPLE_HPP
#define INCLUDED_UHD_TRANSPORT_UDP_SIMPLE_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

class UHD_API udp_simple : boost::noncopyable{
public:
    typedef boost::shared_ptr<udp_simple> sptr;

    virtual ~udp_simple(void) = 0;

    //! The maximum number of bytes per udp packet.
    static const size_t mtu = 1500 - 20 - 8; //default ipv4 mtu - ipv4 header - udp header

    /*!
     * Make a new connected udp transport:
     * This transport is for sending and receiving
     * between this host and a single endpoint.
     * The primary usage for this transport will be control transactions.
     * The underlying implementation is simple and portable (not fast).
     *
     * The address will be resolved, it can be a host name or ipv4.
     * The port will be resolved, it can be a port type or number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_connected(const std::string &addr, const std::string &port);

    /*!
     * Make a new broadcasting udp transport:
     * This transport can send udp broadcast datagrams
     * and receive datagrams from multiple sources.
     * The primary usage for this transport will be to discover devices.
     *
     * The address will be resolved, it can be a host name or ipv4.
     * The port will be resolved, it can be a port type or number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_broadcast(const std::string &addr, const std::string &port);

    /*!
     * Make a UART interface from a UDP transport.
     * \param udp the UDP transport object
     * \return a new UART interface
     */
    static uart_iface::sptr make_uart(sptr udp);

    /*!
     * Send a single buffer.
     * Blocks until the data is sent.
     * \param buff single asio buffer
     * \return the number of bytes sent
     */
    virtual size_t send(const boost::asio::const_buffer &buff) = 0;

    /*!
     * Receive into the provided buffer.
     * Blocks until data is received or a timeout occurs.
     * \param buff a mutable buffer to receive into
     * \param timeout the timeout in seconds
     * \return the number of bytes received or zero on timeout
     */
    virtual size_t recv(const boost::asio::mutable_buffer &buff, double timeout = 0.1) = 0;

    /*!
     * Get the last IP address as seen by recv().
     * Only use this with the broadcast socket.
     */
    virtual std::string get_recv_addr(void) = 0;

    /*!
     * Get the IP address for the destination
     */
    virtual std::string get_send_addr(void) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_UDP_SIMPLE_HPP */
