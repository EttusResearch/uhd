//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_RX_DSP_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_RX_DSP_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>

class rx_dsp_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<rx_dsp_core_200> sptr;

    virtual ~rx_dsp_core_200(void) = 0;

    static sptr make(
        uhd::wb_iface::sptr iface,
        const size_t dsp_base, const size_t ctrl_base,
        const uint32_t sid, const bool lingering_packet = false
    );

    virtual void clear(void) = 0;

    virtual void set_nsamps_per_packet(const size_t nsamps) = 0;

    virtual void issue_stream_command(const uhd::stream_cmd_t &stream_cmd) = 0;

    virtual void set_mux(const std::string &mode, const bool fe_swapped = false) = 0;

    virtual void set_tick_rate(const double rate) = 0;

    virtual void set_link_rate(const double rate) = 0;

    virtual double set_host_rate(const double rate) = 0;

    virtual uhd::meta_range_t get_host_rates(void) = 0;

    virtual double get_scaling_adjustment(void) = 0;

    virtual uhd::meta_range_t get_freq_range(void) = 0;

    virtual double set_freq(const double freq) = 0;

    virtual void handle_overflow(void) = 0;

    virtual void setup(const uhd::stream_args_t &stream_args) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_RX_DSP_CORE_200_HPP */
