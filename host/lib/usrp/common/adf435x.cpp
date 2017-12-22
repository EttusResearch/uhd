//
// Copyright 2013-2014 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#include "adf435x.hpp"

using namespace uhd;

adf435x_iface::~adf435x_iface()
{
}

adf435x_iface::sptr adf435x_iface::make_adf4350(write_fn_t write)
{
    return sptr(new adf435x_impl<adf4350_regs_t>(write));
}

adf435x_iface::sptr adf435x_iface::make_adf4351(write_fn_t write)
{
    return sptr(new adf435x_impl<adf4351_regs_t>(write));
}
