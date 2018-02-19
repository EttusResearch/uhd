//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
 * On Linux systems, the zero copy transport can use a kernel packet ring.
 * If no platform specific solution is available, make returns a boost asio
 * implementation that wraps the functionality around a standard send/recv calls.
 */
class UHD_API udp_zero_copy : public virtual zero_copy_if{
public:
    struct buff_params {
        size_t  recv_buff_size;
        size_t  send_buff_size;
    };

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
     * \param default_buff_args Default values for frame sizes and num frames
     * \param[out] buff_params_out Returns the actual buffer sizes
     * \param hints optional parameters to pass to the underlying transport
     */
    static sptr make(
        const std::string &addr,
        const std::string &port,
        const zero_copy_xport_params &default_buff_args,
        udp_zero_copy::buff_params& buff_params_out,
        const device_addr_t &hints = device_addr_t()
    );

    /*! Return the local port of the UDP connection
     *
     * Port is in host byte order. No funny business here.
     *
     * \returns Port number or 0 if port number couldn't be identified.
     */
    virtual uint16_t get_local_port(void) const = 0;

    /*! Return the local IP address of the UDP connection as a dotted string.
     *
     * \returns IP address as a string or empty string if the IP address could
     *          not be identified.
     */
    virtual std::string get_local_addr(void) const = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_UDP_ZERO_COPY_HPP */
