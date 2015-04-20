//
// Copyright 2010-2013,2015 Ettus Research LLC
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

#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>

using namespace uhd::usrp;

struct dboard_iface::impl{
    uhd::dict<unit_t, boost::uint16_t> pin_ctrl_shadow;
    uhd::dict<unit_t, uhd::dict<atr_reg_t, boost::uint16_t> > atr_reg_shadow;
    uhd::dict<unit_t, boost::uint16_t> gpio_ddr_shadow;
    uhd::dict<unit_t, boost::uint16_t> gpio_out_shadow;
};

dboard_iface::dboard_iface(void){
    _impl = UHD_PIMPL_MAKE(impl, ());
}

dboard_iface::~dboard_iface(void)
{
    //empty
}

template <typename T>
static T shadow_it(T &shadow, const T &value, const T &mask){
    shadow = (shadow & ~mask) | (value & mask);
    return shadow;
}

void dboard_iface::set_pin_ctrl(
    unit_t unit, boost::uint16_t value, boost::uint16_t mask
){
    _set_pin_ctrl(unit, shadow_it(_impl->pin_ctrl_shadow[unit], value, mask));
}

boost::uint16_t dboard_iface::get_pin_ctrl(unit_t unit){
    return _impl->pin_ctrl_shadow[unit];
}

void dboard_iface::set_atr_reg(
    unit_t unit, atr_reg_t reg, boost::uint16_t value, boost::uint16_t mask
){
    _set_atr_reg(unit, reg, shadow_it(_impl->atr_reg_shadow[unit][reg], value, mask));
}

boost::uint16_t dboard_iface::get_atr_reg(unit_t unit, atr_reg_t reg){
    return _impl->atr_reg_shadow[unit][reg];
}

void dboard_iface::set_gpio_ddr(
    unit_t unit, boost::uint16_t value, boost::uint16_t mask
){
    _set_gpio_ddr(unit, shadow_it(_impl->gpio_ddr_shadow[unit], value, mask));
}

boost::uint16_t dboard_iface::get_gpio_ddr(unit_t unit){
    return _impl->gpio_ddr_shadow[unit];
}

void dboard_iface::set_gpio_out(
    unit_t unit, boost::uint16_t value, boost::uint16_t mask
){
    _set_gpio_out(unit, shadow_it(_impl->gpio_out_shadow[unit], value, mask));
}

boost::uint16_t dboard_iface::get_gpio_out(unit_t unit){
    return _impl->gpio_out_shadow[unit];
}

void dboard_iface::set_command_time(const uhd::time_spec_t&)
{
    throw uhd::not_implemented_error("timed command feature not implemented on this hardware");
}

uhd::time_spec_t dboard_iface::get_command_time()
{
    return uhd::time_spec_t(0.0);
}
