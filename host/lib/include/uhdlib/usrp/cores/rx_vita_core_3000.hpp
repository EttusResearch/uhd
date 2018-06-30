//
// Copyright 2013,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_RX_VITA_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_RX_VITA_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>

class rx_vita_core_3000 : boost::noncopyable
{
public:
    typedef boost::shared_ptr<rx_vita_core_3000> sptr;

    virtual ~rx_vita_core_3000(void) = 0;

    static sptr make(
        uhd::wb_iface::sptr iface,
        const size_t base
    );

    virtual void clear(void) = 0;

    virtual void set_nsamps_per_packet(const size_t nsamps) = 0;

    virtual void issue_stream_command(const uhd::stream_cmd_t &stream_cmd) = 0;

    virtual void set_tick_rate(const double rate) = 0;

    virtual void set_sid(const uint32_t sid) = 0;

    virtual void handle_overflow(void) = 0;

    virtual void setup(const uhd::stream_args_t &stream_args) = 0;

    virtual void configure_flow_control(const size_t window_size) = 0;

    virtual bool in_continuous_streaming_mode(void) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_RX_VITA_CORE_3000_HPP */
