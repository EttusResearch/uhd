//
// Copyright 2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRP2_FIFO_CTRL_HPP
#define INCLUDED_USRP2_FIFO_CTRL_HPP

#include <uhd/types/time_spec.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>

/*!
 * The usrp2 FIFO control class:
 * Provide high-speed peek/poke interface.
 */
class usrp2_fifo_ctrl : public uhd::timed_wb_iface, public uhd::spi_iface
{
public:
    typedef boost::shared_ptr<usrp2_fifo_ctrl> sptr;

    //! Make a new FIFO control object
    static sptr make(uhd::transport::zero_copy_if::sptr xport);

    //! Set the tick rate (converting time into ticks)
    virtual void set_tick_rate(const double rate) = 0;
};

#endif /* INCLUDED_USRP2_FIFO_CTRL_HPP */
