//
// Copyright 2011 Ettus Research LLC
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

#include "gpio_core_200.hpp"
#include <uhd/types/dict.hpp>

#define REG_GPIO_IDLE          _base + 0
#define REG_GPIO_RX_ONLY       _base + 4
#define REG_GPIO_TX_ONLY       _base + 8
#define REG_GPIO_BOTH          _base + 12
#define REG_GPIO_DDR           _base + 16

using namespace uhd;
using namespace usrp;

class gpio_core_200_impl : public gpio_core_200{
public:
    gpio_core_200_impl(wb_iface::sptr iface, const size_t base, const size_t rb_addr):
        _iface(iface), _base(base), _rb_addr(rb_addr) { /* NOP */ }

    void set_pin_ctrl(const unit_t unit, const boost::uint16_t value){
        _pin_ctrl[unit] = value; //shadow
        this->update(); //full update
    }

    void set_atr_reg(const unit_t unit, const atr_reg_t atr, const boost::uint16_t value){
        _atr_regs[unit][atr] = value;  //shadow
        this->update(); //full update
    }

    void set_gpio_ddr(const unit_t unit, const boost::uint16_t value){
        _gpio_ddr[unit] = value; //shadow
        _iface->poke32(REG_GPIO_DDR, //update the 32 bit register
            (boost::uint32_t(_gpio_ddr[dboard_iface::UNIT_RX]) << unit2shit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_gpio_ddr[dboard_iface::UNIT_TX]) << unit2shit(dboard_iface::UNIT_TX))
        );
    }

    void set_gpio_out(const unit_t unit, const boost::uint16_t value){
        _gpio_out[unit] = value; //shadow
        this->update(); //full update
    }

    boost::uint16_t read_gpio(const unit_t unit){
        return boost::uint16_t(_iface->peek32(_rb_addr) >> unit2shit(unit));
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _rb_addr;
    uhd::dict<size_t, boost::uint32_t> _update_cache;

    uhd::dict<unit_t, boost::uint16_t> _pin_ctrl, _gpio_out, _gpio_ddr;
    uhd::dict<unit_t, uhd::dict<atr_reg_t, boost::uint16_t> > _atr_regs;

    unsigned unit2shit(const unit_t unit){
        return (unit == dboard_iface::UNIT_RX)? 0 : 16;
    }

    void update(void){
        this->update(dboard_iface::ATR_REG_IDLE, REG_GPIO_IDLE);
        this->update(dboard_iface::ATR_REG_TX_ONLY, REG_GPIO_TX_ONLY);
        this->update(dboard_iface::ATR_REG_RX_ONLY, REG_GPIO_RX_ONLY);
        this->update(dboard_iface::ATR_REG_FULL_DUPLEX, REG_GPIO_BOTH);
    }

    void update(const atr_reg_t atr, const size_t addr){
        const boost::uint32_t atr_val =
            (boost::uint32_t(_atr_regs[dboard_iface::UNIT_RX][atr]) << unit2shit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_atr_regs[dboard_iface::UNIT_TX][atr]) << unit2shit(dboard_iface::UNIT_TX));

        const boost::uint32_t gpio_val =
            (boost::uint32_t(_gpio_out[dboard_iface::UNIT_RX]) << unit2shit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_gpio_out[dboard_iface::UNIT_TX]) << unit2shit(dboard_iface::UNIT_TX));

        const boost::uint32_t ctrl =
            (boost::uint32_t(_pin_ctrl[dboard_iface::UNIT_RX]) << unit2shit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_pin_ctrl[dboard_iface::UNIT_TX]) << unit2shit(dboard_iface::UNIT_TX));
        const boost::uint32_t val = (ctrl & atr_val) | ((~ctrl) & gpio_val);
        if (not _update_cache.has_key(addr) or _update_cache[addr] != val)
        {
            _iface->poke32(addr, val);
        }
        _update_cache[addr] = val;
    }

};

gpio_core_200::sptr gpio_core_200::make(wb_iface::sptr iface, const size_t base, const size_t rb_addr){
    return sptr(new gpio_core_200_impl(iface, base, rb_addr));
}
