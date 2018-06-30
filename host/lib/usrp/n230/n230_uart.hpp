//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_UART_HPP
#define INCLUDED_N230_UART_HPP

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/serial.hpp> //uart iface
#include <uhd/utils/tasks.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
namespace uhd { namespace usrp { namespace n230 {

class n230_uart: boost::noncopyable, public uhd::uart_iface
{
public:
    typedef boost::shared_ptr<n230_uart> sptr;
    static sptr make(uhd::transport::zero_copy_if::sptr, const uint32_t sid);
    virtual void set_baud_divider(const double baud_div) = 0;
};

}}} //namespace

#endif /* INCLUDED_N230_UART_HPP */
