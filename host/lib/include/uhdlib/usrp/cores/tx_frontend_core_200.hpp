//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_RX_FRONTEND_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_RX_FRONTEND_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/property_tree.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <complex>
#include <string>

class tx_frontend_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<tx_frontend_core_200> sptr;

    static const std::complex<double> DEFAULT_DC_OFFSET_VALUE;
    static const std::complex<double> DEFAULT_IQ_BALANCE_VALUE;

    virtual ~tx_frontend_core_200(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_mux(const std::string &mode) = 0;

    virtual std::complex<double> set_dc_offset(const std::complex<double> &off) = 0;

    virtual void set_iq_balance(const std::complex<double> &cor) = 0;

    virtual void populate_subtree(uhd::property_tree::sptr subtree) = 0;

};

#endif /* INCLUDED_LIBUHD_USRP_RX_FRONTEND_CORE_200_HPP */
