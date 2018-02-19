//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/static.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>

namespace uhd { namespace usrp {

/***********************************************************************
 * The E310 dboard
 *   AD9361 Interface, thus two subdevs
 **********************************************************************/
class e310_dboard : public xcvr_dboard_base{
public:
    e310_dboard(ctor_args_t args) : xcvr_dboard_base(args) {}

    virtual ~e310_dboard(void) {}
};

/***********************************************************************
 * The E310 dboard
 *   AD9364 Interface, thus one subdev
 **********************************************************************/
class e300_dboard : public xcvr_dboard_base{
public:
    e300_dboard(ctor_args_t args) : xcvr_dboard_base(args) {}

    virtual ~e300_dboard(void) {}
};

/***********************************************************************
 * Register the E310 dboards
 **********************************************************************/
static dboard_base::sptr make_e310_dboard(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new e310_dboard(args));
}

static dboard_base::sptr make_e300_dboard(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new e300_dboard(args));
}

}} // namespace

using namespace uhd::usrp;

UHD_STATIC_BLOCK(reg_e3x0_dboards){
    dboard_manager::register_dboard(0x0110, &make_e310_dboard, "E310 MIMO XCVR");
    dboard_manager::register_dboard(0x0100, &make_e300_dboard, "E300 SISO XCVR");
}
