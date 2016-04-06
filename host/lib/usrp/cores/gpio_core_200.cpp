//
// Copyright 2011,2014 Ettus Research LLC
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

gpio_core_200::~gpio_core_200(void){
    /* NOP */
}

class gpio_core_200_impl : public gpio_core_200{
public:
    gpio_core_200_impl(wb_iface::sptr iface, const size_t base, const size_t rb_addr):
        _iface(iface), _base(base), _rb_addr(rb_addr), _first_atr(true) { /* NOP */ }

    void set_pin_ctrl(const unit_t unit, const boost::uint16_t value){
        _pin_ctrl[unit] = value; //shadow
        update(); //full update
    }

    void set_atr_reg(const unit_t unit, const atr_reg_t atr, const boost::uint16_t value){
        _atr_regs[unit][atr] = value;  //shadow
        if (_first_atr)
        {
            // To preserve legacy behavior, update all registers the first time
            update();
            _first_atr = false;
        }
        else
            update(atr);
    }

    void set_gpio_ddr(const unit_t unit, const boost::uint16_t value){
        _gpio_ddr[unit] = value; //shadow
        _iface->poke32(REG_GPIO_DDR, //update the 32 bit register
            (boost::uint32_t(_gpio_ddr[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_gpio_ddr[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX))
        );
    }

    void set_gpio_out(const unit_t unit, const boost::uint16_t value){
        _gpio_out[unit] = value; //shadow
        this->update(); //full update
    }

    boost::uint16_t read_gpio(const unit_t unit){
        return boost::uint16_t(_iface->peek32(_rb_addr) >> shift_by_unit(unit));
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _rb_addr;
    bool _first_atr;
    uhd::dict<size_t, boost::uint32_t> _update_cache;

    uhd::dict<unit_t, boost::uint16_t> _pin_ctrl, _gpio_out, _gpio_ddr;
    uhd::dict<unit_t, uhd::dict<atr_reg_t, boost::uint16_t> > _atr_regs;

    unsigned shift_by_unit(const unit_t unit){
        return (unit == dboard_iface::UNIT_RX)? 0 : 16;
    }

    void update(void){
        update(dboard_iface::ATR_REG_IDLE);
        update(dboard_iface::ATR_REG_TX_ONLY);
        update(dboard_iface::ATR_REG_RX_ONLY);
        update(dboard_iface::ATR_REG_FULL_DUPLEX);
    }

    void update(const atr_reg_t atr){
        size_t addr;
        switch (atr)
        {
        case dboard_iface::ATR_REG_IDLE:
            addr = REG_GPIO_IDLE;
            break;
        case dboard_iface::ATR_REG_TX_ONLY:
            addr = REG_GPIO_TX_ONLY;
            break;
        case dboard_iface::ATR_REG_RX_ONLY:
            addr = REG_GPIO_RX_ONLY;
            break;
        case dboard_iface::ATR_REG_FULL_DUPLEX:
            addr = REG_GPIO_BOTH;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
        const boost::uint32_t atr_val =
            (boost::uint32_t(_atr_regs[dboard_iface::UNIT_RX][atr]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_atr_regs[dboard_iface::UNIT_TX][atr]) << shift_by_unit(dboard_iface::UNIT_TX));

        const boost::uint32_t gpio_val =
            (boost::uint32_t(_gpio_out[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_gpio_out[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX));

        const boost::uint32_t ctrl =
            (boost::uint32_t(_pin_ctrl[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (boost::uint32_t(_pin_ctrl[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX));
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

gpio_core_200_32wo::~gpio_core_200_32wo(void){
    /* NOP */
}

class gpio_core_200_32wo_impl : public gpio_core_200_32wo{
public:
    gpio_core_200_32wo_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base)
    {
        _iface->poke32(REG_GPIO_DDR, 0xffffffff);
    }

    void set_atr_reg(const atr_reg_t atr, const boost::uint32_t value){
        if (atr == dboard_iface::ATR_REG_IDLE)
            _iface->poke32(REG_GPIO_IDLE, value);
        else if (atr == dboard_iface::ATR_REG_TX_ONLY)
            _iface->poke32(REG_GPIO_TX_ONLY, value);
        else if (atr == dboard_iface::ATR_REG_RX_ONLY)
            _iface->poke32(REG_GPIO_RX_ONLY, value);
        else if (atr == dboard_iface::ATR_REG_FULL_DUPLEX)
            _iface->poke32(REG_GPIO_BOTH, value);
        else
            UHD_THROW_INVALID_CODE_PATH();
    }

    void set_all_regs(const boost::uint32_t value){
        set_atr_reg(dboard_iface::ATR_REG_IDLE,        value);
        set_atr_reg(dboard_iface::ATR_REG_TX_ONLY,     value);
        set_atr_reg(dboard_iface::ATR_REG_RX_ONLY,     value);
        set_atr_reg(dboard_iface::ATR_REG_FULL_DUPLEX, value);
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;

};

gpio_core_200_32wo::sptr gpio_core_200_32wo::make(wb_iface::sptr iface, const size_t base){
    return sptr(new gpio_core_200_32wo_impl(iface, base));
}
