//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <complex>
#include <memory>
#include <string>

class tx_frontend_core_200 : uhd::noncopyable
{
public:
    typedef std::shared_ptr<tx_frontend_core_200> sptr;

    static const std::complex<double> DEFAULT_DC_OFFSET_VALUE;
    static const std::complex<double> DEFAULT_IQ_BALANCE_VALUE;

    virtual ~tx_frontend_core_200(void) = 0;

    static sptr make(
        uhd::wb_iface::sptr iface, const size_t base, const size_t offset = 4);

    virtual void set_mux(const std::string& mode) = 0;

    virtual std::complex<double> set_dc_offset(const std::complex<double>& off) = 0;

    virtual void set_iq_balance(const std::complex<double>& cor) = 0;

    virtual void populate_subtree(uhd::property_tree::sptr subtree) = 0;
};
