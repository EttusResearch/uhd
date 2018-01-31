//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_TX_VITA_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_TX_VITA_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>

class tx_vita_core_3000 : boost::noncopyable
{
public:
    typedef boost::shared_ptr<tx_vita_core_3000> sptr;

    enum fc_monitor_loc {
        FC_DEFAULT,
        FC_PRE_RADIO,
        FC_PRE_FIFO
    };

    virtual ~tx_vita_core_3000(void) = 0;

    static sptr make(
        uhd::wb_iface::sptr iface,
        const size_t base,
        fc_monitor_loc fc_location = FC_PRE_RADIO
    );

    static sptr make_no_radio_buff(
        uhd::wb_iface::sptr iface,
        const size_t base
    );

    virtual void clear(void) = 0;

    virtual void setup(const uhd::stream_args_t &stream_args) = 0;

    virtual void configure_flow_control(const size_t cycs_per_up, const size_t pkts_per_up) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_TX_VITA_CORE_3000_HPP */
