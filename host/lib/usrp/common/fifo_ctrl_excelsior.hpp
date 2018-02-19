//
// Copyright 2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B200_CTRL_HPP
#define INCLUDED_B200_CTRL_HPP

#include <uhd/types/time_spec.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>


struct fifo_ctrl_excelsior_config
{
    size_t async_sid_base;
    size_t num_async_chan;
    size_t ctrl_sid_base;
    size_t spi_base;
    size_t spi_rb;
};

/*!
 * Provide access to peek, poke, spi, and async messages.
 */
class fifo_ctrl_excelsior : public uhd::timed_wb_iface, public uhd::spi_iface
{
public:
    typedef boost::shared_ptr<fifo_ctrl_excelsior> sptr;

    //! Make a new control object
    static sptr make(
        uhd::transport::zero_copy_if::sptr xport,
        const fifo_ctrl_excelsior_config &config
    );

    //! Set the tick rate (converting time into ticks)
    virtual void set_tick_rate(const double rate) = 0;

    //! Pop an async message from the queue or timeout
    virtual bool pop_async_msg(uhd::async_metadata_t &async_metadata, double timeout) = 0;
};

#endif /* INCLUDED_B200_CTRL_HPP */
