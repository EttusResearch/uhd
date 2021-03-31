//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <tuple>

namespace uhd { namespace transport {

enum class link_type_t { CTRL = 0, ASYNC_MSG, TX_DATA, RX_DATA };

//! Contains all information regarding a link interface
using both_links_t = std::tuple<uhd::transport::send_link_if::sptr,
    size_t, // num_send_frames
    uhd::transport::recv_link_if::sptr,
    size_t, // num_recv_frames
    bool, // lossy_xport
    bool, // packet flow control
    bool>; // enable flow control

/*!
 * Parameters for link creation.
 */
struct link_params_t
{
    size_t recv_frame_size = 0;
    size_t send_frame_size = 0;
    size_t num_recv_frames = 0;
    size_t num_send_frames = 0;
    size_t recv_buff_size  = 0;
    size_t send_buff_size  = 0;
};


}} // namespace uhd::transport
