//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>

namespace uhd{ namespace usrp{

    class recv_packet_demuxer{
    public:
        typedef boost::shared_ptr<recv_packet_demuxer> sptr;

        virtual ~recv_packet_demuxer(void) = 0;

        //! Make a new demuxer from a transport and parameters
        static sptr make(transport::zero_copy_if::sptr transport, const size_t size, const uint32_t sid_base);

        //! Get a buffer at the given index from the transport
        virtual transport::managed_recv_buffer::sptr get_recv_buff(const size_t index, const double timeout) = 0;
    };

}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_HPP */
