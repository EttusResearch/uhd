//
// Copyright 2011-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_TIME64_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_TIME64_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>
#include <vector>

class time64_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<time64_core_200> sptr;

    struct readback_bases_type{
        size_t rb_hi_now, rb_lo_now;
        size_t rb_hi_pps, rb_lo_pps;
    };

    virtual ~time64_core_200(void) = 0;

    //! makes a new time64 core from iface and slave base
    static sptr make(
        uhd::wb_iface::sptr iface, const size_t base,
        const readback_bases_type &readback_bases,
        const size_t mimo_delay_cycles = 0 // 0 means no-mimo
    );

    virtual void enable_gpsdo(void) = 0;

    virtual void set_tick_rate(const double rate) = 0;

    virtual uhd::time_spec_t get_time_now(void) = 0;

    virtual uhd::time_spec_t get_time_last_pps(void) = 0;

    virtual void set_time_now(const uhd::time_spec_t &time) = 0;

    virtual void set_time_next_pps(const uhd::time_spec_t &time) = 0;

    virtual void set_time_source(const std::string &source) = 0;

    virtual std::vector<std::string> get_time_sources(void) = 0;

};

#endif /* INCLUDED_LIBUHD_USRP_TIME64_CORE_200_HPP */
