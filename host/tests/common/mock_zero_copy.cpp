//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include "mock_zero_copy.hpp"
#include <memory>

using namespace uhd::transport;

mock_zero_copy::mock_zero_copy(vrt::if_packet_info_t::link_type_t link_type,
    size_t recv_frame_size,
    size_t send_frame_size)
    : _link_type(link_type)
    , _recv_frame_size(recv_frame_size)
    , _send_frame_size(send_frame_size)
{
}

uhd::transport::managed_recv_buffer::sptr mock_zero_copy::get_recv_buff(double)
{
    if (_simulate_io_error) {
        throw uhd::io_error("IO error exception"); // simulate an IO error
    }
    if (_rx_mems.empty()) {
        return uhd::transport::managed_recv_buffer::sptr(); // timeout
    }

    uhd::transport::managed_recv_buffer::sptr mrb =
        _mrb.get_new(_rx_mems.front(), _rx_lens.front());

    if (not _reuse_recv_memory) {
        _rx_mems.pop_front();
        _rx_lens.pop_front();
    }

    return mrb;
}

uhd::transport::managed_send_buffer::sptr mock_zero_copy::get_send_buff(double)
{
    if (not _reuse_send_memory or _tx_mems.empty()) {
        _tx_mems.push_back(boost::shared_array<uint8_t>(new uint8_t[_send_frame_size]));
        _tx_lens.push_back(_send_frame_size);
    }

    return _msb.get_new(_tx_mems.back(), &_tx_lens.back());
}

void mock_zero_copy::set_reuse_recv_memory(bool reuse_recv)
{
    _reuse_recv_memory = reuse_recv;
}

void mock_zero_copy::set_reuse_send_memory(bool reuse_send)
{
    _reuse_send_memory = reuse_send;
}
