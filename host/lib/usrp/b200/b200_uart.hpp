//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B200_UART_HPP
#define INCLUDED_B200_UART_HPP

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/serial.hpp> //uart iface
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class b200_uart: boost::noncopyable, public uhd::uart_iface
{
public:
    typedef boost::shared_ptr<b200_uart> sptr;
    static sptr make(uhd::transport::zero_copy_if::sptr, const uint32_t sid);
    virtual void handle_uart_packet(uhd::transport::managed_recv_buffer::sptr buff) = 0;
};


#endif /* INCLUDED_B200_UART_HPP */
