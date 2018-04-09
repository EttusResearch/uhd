//
// Copyright 2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/cores/user_settings_core_200.hpp>

using namespace uhd;

#define REG_USER_ADDR             _base + 0
#define REG_USER_DATA             _base + 4

user_settings_core_200::~user_settings_core_200(void){
    /* NOP */
}

class user_settings_core_200_impl : public user_settings_core_200{
public:
    user_settings_core_200_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base)
    {
        //NOP
    }

    void set_reg(const user_reg_t &reg){
        _iface->poke32(REG_USER_ADDR, reg.first);
        _iface->poke32(REG_USER_DATA, reg.second);
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
};

user_settings_core_200::sptr user_settings_core_200::make(wb_iface::sptr iface, const size_t base){
    return sptr(new user_settings_core_200_impl(iface, base));
}
