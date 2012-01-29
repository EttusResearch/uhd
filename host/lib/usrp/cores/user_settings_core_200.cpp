//
// Copyright 2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "user_settings_core_200.hpp"

#define REG_USER_ADDR             _base + 0
#define REG_USER_DATA             _base + 4

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
