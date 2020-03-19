//
// Copyright 2011,2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/fe_connection.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <complex>
#include <memory>
#include <string>

class rx_frontend_core_3000 : uhd::noncopyable
{
public:
    static const std::complex<double> DEFAULT_DC_OFFSET_VALUE;
    static const bool DEFAULT_DC_OFFSET_ENABLE;
    static const std::complex<double> DEFAULT_IQ_BALANCE_VALUE;

    typedef std::shared_ptr<rx_frontend_core_3000> sptr;

    virtual ~rx_frontend_core_3000(void) = 0;

    static sptr make(
        uhd::wb_iface::sptr iface, const size_t base, const size_t reg_offset = 4);

    /*! Set the input sampling rate (i.e. ADC rate)
     */
    virtual void set_adc_rate(const double rate) = 0;

    virtual void bypass_all(bool bypass_en) = 0;

    virtual void set_fe_connection(const uhd::usrp::fe_connection_t& fe_conn) = 0;

    virtual void set_dc_offset_auto(const bool enb) = 0;

    virtual std::complex<double> set_dc_offset(const std::complex<double>& off) = 0;

    virtual void set_iq_balance(const std::complex<double>& cor) = 0;

    virtual void populate_subtree(uhd::property_tree::sptr subtree) = 0;
};
