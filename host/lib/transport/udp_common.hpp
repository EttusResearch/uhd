//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP

#include <uhd/config.hpp>
#include <boost/asio.hpp>

namespace uhd{ namespace transport{

    // Jumbo frames are limited to 9000;
    static const size_t MAX_ETHERNET_MTU = 9000;

    typedef boost::shared_ptr<boost::asio::ip::udp::socket> socket_sptr;

    /*!
     * Wait for the socket to become ready for a receive operation.
     * \param sock_fd the open socket file descriptor
     * \param timeout the timeout duration in seconds
     * \return true when the socket is ready for receive
     */
    UHD_INLINE bool wait_for_recv_ready(int sock_fd, double timeout){
#ifdef UHD_PLATFORM_WIN32 // select is more portable than poll unfortunately
        //setup timeval for timeout
        timeval tv;
        //If the tv_usec > 1 second on some platforms, select will
        //error EINVAL: An invalid timeout interval was specified.
        tv.tv_sec = int(timeout);
        tv.tv_usec = int(timeout*1000000)%1000000;

        //setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(sock_fd, &rset);

        //http://www.gnu.org/s/hello/manual/libc/Interrupted-Primitives.html
        //This macro is provided with gcc to properly deal with EINTR.
        //If not provided, define an empty macro, assume that is OK
        #ifndef TEMP_FAILURE_RETRY
            #define TEMP_FAILURE_RETRY(x) (x)
        #endif

        //call select with timeout on receive socket
        return TEMP_FAILURE_RETRY(::select(sock_fd+1, &rset, NULL, NULL, &tv)) > 0;
#else
        //calculate the total timeout in milliseconds (from seconds)
        int total_timeout = int(timeout*1000);

        pollfd pfd_read;
        pfd_read.fd = sock_fd;
        pfd_read.events = POLLIN;

        //call poll with timeout on receive socket
        return ::poll(&pfd_read, 1, total_timeout) > 0;
#endif
    }

}} //namespace uhd::transport

#endif /* INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP */
