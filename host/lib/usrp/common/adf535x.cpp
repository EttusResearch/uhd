//
// Copyright 2013-2014, 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "adf535x.hpp"

adf535x_iface::sptr adf535x_iface::make_adf5355(write_fn_t write)
{
    return sptr(new adf535x_impl<adf5355_regs_t>(write));
}

adf535x_iface::sptr adf535x_iface::make_adf5356(write_fn_t write)
{
    return sptr(new adf535x_impl<adf5356_regs_t>(write));
}