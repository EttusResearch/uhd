//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include "mock_zero_copy.hpp"
#include <boost/shared_ptr.hpp>

uhd::transport::managed_recv_buffer::sptr mock_zero_copy::get_recv_buff(double) {
    if (_rx_mems.empty()) {
        return uhd::transport::managed_recv_buffer::sptr(); // timeout
    }
    _mrbs.push_back(boost::make_shared<mock_mrb>());
    uhd::transport::managed_recv_buffer::sptr mrb =
        _mrbs.back()->get_new(_rx_mems.front(), _rx_lens.front());
    _rx_mems.pop_front();
    _rx_lens.pop_front();
    return mrb;
}

uhd::transport::managed_send_buffer::sptr mock_zero_copy::get_send_buff(double) {
    _msbs.push_back(boost::make_shared<mock_msb>());
    _tx_mems.push_back(
        boost::shared_array<uint8_t>(new uint8_t[SEND_BUFF_SIZE]));
    _tx_lens.push_back(SEND_BUFF_SIZE);
    return _msbs.back()->get_new(_tx_mems.back(), &_tx_lens.back());
}
