//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_XPORTS_HPP
#define INCLUDED_LIBUHD_XPORTS_HPP

#include <uhd/types/sid.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/transport/zero_copy.hpp>

namespace uhd {

    /*! Holds all necessary items for a bidirectional link
     */
    struct both_xports_t
    {
        both_xports_t(): recv_buff_size(0), send_buff_size(0) {}
        uhd::transport::zero_copy_if::sptr recv;
        uhd::transport::zero_copy_if::sptr send;
        size_t recv_buff_size;
        size_t send_buff_size;
        uhd::sid_t send_sid;
        uhd::sid_t recv_sid;
        uhd::endianness_t endianness;
    };

};

#endif /* INCLUDED_LIBUHD_XPORTS_HPP */

