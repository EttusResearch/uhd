//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhdlib/usrp/cores/gpio_core_200.hpp>

#define REG_GPIO_IDLE          _base + 0
#define REG_GPIO_RX_ONLY       _base + 4
#define REG_GPIO_TX_ONLY       _base + 8
#define REG_GPIO_BOTH          _base + 12
#define REG_GPIO_DDR           _base + 16

using namespace uhd;
using namespace usrp;

template <typename T>
static void shadow_it(T &shadow, const T &value, const T &mask){
    shadow = (shadow & ~mask) | (value & mask);
}

gpio_core_200::~gpio_core_200(void){
    /* NOP */
}

class gpio_core_200_impl : public gpio_core_200{
public:
    gpio_core_200_impl(wb_iface::sptr iface, const size_t base, const size_t rb_addr):
        _iface(iface), _base(base), _rb_addr(rb_addr), _first_atr(true) { /* NOP */ }

    void set_pin_ctrl(const unit_t unit, const uint16_t value, const uint16_t mask){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        shadow_it(_pin_ctrl[unit], value, mask);
        update(); //full update
    }

    uint16_t get_pin_ctrl(unit_t unit){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        return _pin_ctrl[unit];
    }

    void set_atr_reg(const unit_t unit, const atr_reg_t atr, const uint16_t value, const uint16_t mask){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        shadow_it(_atr_regs[unit][atr], value, mask);
        if (_first_atr)
        {
            // To preserve legacy behavior, update all registers the first time
            update();
            _first_atr = false;
        }
        else
            update(atr);
    }

    uint16_t get_atr_reg(unit_t unit, atr_reg_t reg){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        return _atr_regs[unit][reg];
    }

    void set_gpio_ddr(const unit_t unit, const uint16_t value, const uint16_t mask){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        shadow_it(_gpio_ddr[unit], value, mask);
        _iface->poke32(REG_GPIO_DDR, //update the 32 bit register
            (uint32_t(_gpio_ddr[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (uint32_t(_gpio_ddr[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX))
        );
    }

    uint16_t get_gpio_ddr(unit_t unit){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        return _gpio_ddr[unit];
    }

    void set_gpio_out(const unit_t unit, const uint16_t value, const uint16_t mask){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        shadow_it(_gpio_out[unit], value, mask);
        this->update(); //full update
    }

    uint16_t get_gpio_out(unit_t unit){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        return _gpio_out[unit];
    }

    uint16_t read_gpio(const unit_t unit){
        if (unit == dboard_iface::UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported in gpio_core_200");
        return uint16_t(_iface->peek32(_rb_addr) >> shift_by_unit(unit));
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _rb_addr;
    bool _first_atr;
    uhd::dict<size_t, uint32_t> _update_cache;

    uhd::dict<unit_t, uint16_t> _pin_ctrl, _gpio_out, _gpio_ddr;
    uhd::dict<unit_t, uhd::dict<atr_reg_t, uint16_t> > _atr_regs;

    unsigned shift_by_unit(const unit_t unit){
        return (unit == dboard_iface::UNIT_RX)? 0 : 16;
    }

    void update(void){
        update(gpio_atr::ATR_REG_IDLE);
        update(gpio_atr::ATR_REG_TX_ONLY);
        update(gpio_atr::ATR_REG_RX_ONLY);
        update(gpio_atr::ATR_REG_FULL_DUPLEX);
    }

    void update(const atr_reg_t atr){
        size_t addr;
        switch (atr)
        {
        case gpio_atr::ATR_REG_IDLE:
            addr = REG_GPIO_IDLE;
            break;
        case gpio_atr::ATR_REG_TX_ONLY:
            addr = REG_GPIO_TX_ONLY;
            break;
        case gpio_atr::ATR_REG_RX_ONLY:
            addr = REG_GPIO_RX_ONLY;
            break;
        case gpio_atr::ATR_REG_FULL_DUPLEX:
            addr = REG_GPIO_BOTH;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
        const uint32_t atr_val =
            (uint32_t(_atr_regs[dboard_iface::UNIT_RX][atr]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (uint32_t(_atr_regs[dboard_iface::UNIT_TX][atr]) << shift_by_unit(dboard_iface::UNIT_TX));

        const uint32_t gpio_val =
            (uint32_t(_gpio_out[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (uint32_t(_gpio_out[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX));

        const uint32_t ctrl =
            (uint32_t(_pin_ctrl[dboard_iface::UNIT_RX]) << shift_by_unit(dboard_iface::UNIT_RX)) |
            (uint32_t(_pin_ctrl[dboard_iface::UNIT_TX]) << shift_by_unit(dboard_iface::UNIT_TX));
        const uint32_t val = (ctrl & atr_val) | ((~ctrl) & gpio_val);
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
        set_ddr_reg();
    }

    void set_ddr_reg(){
        _iface->poke32(REG_GPIO_DDR, 0xffffffff);
    }


    void set_atr_reg(const atr_reg_t atr, const uint32_t value){
        if (atr == gpio_atr::ATR_REG_IDLE)
            _iface->poke32(REG_GPIO_IDLE, value);
        else if (atr == gpio_atr::ATR_REG_TX_ONLY)
            _iface->poke32(REG_GPIO_TX_ONLY, value);
        else if (atr == gpio_atr::ATR_REG_RX_ONLY)
            _iface->poke32(REG_GPIO_RX_ONLY, value);
        else if (atr == gpio_atr::ATR_REG_FULL_DUPLEX)
            _iface->poke32(REG_GPIO_BOTH, value);
        else
            UHD_THROW_INVALID_CODE_PATH();
    }

    void set_all_regs(const uint32_t value){
        set_atr_reg(gpio_atr::ATR_REG_IDLE,        value);
        set_atr_reg(gpio_atr::ATR_REG_TX_ONLY,     value);
        set_atr_reg(gpio_atr::ATR_REG_RX_ONLY,     value);
        set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX, value);
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;

};

gpio_core_200_32wo::sptr gpio_core_200_32wo::make(wb_iface::sptr iface, const size_t base){
    return sptr(new gpio_core_200_32wo_impl(iface, base));
}
