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

#include "gpio_atr_3000.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/utils/soft_register.hpp>

using namespace uhd;
using namespace usrp;

//-------------------------------------------------------------
// gpio_atr_3000
//-------------------------------------------------------------

#define REG_ATR_IDLE_OFFSET     (base + 0)
#define REG_ATR_RX_OFFSET       (base + 4)
#define REG_ATR_TX_OFFSET       (base + 8)
#define REG_ATR_FDX_OFFSET      (base + 12)
#define REG_DDR_OFFSET          (base + 16)
#define REG_ATR_DISABLE_OFFSET  (base + 20)

namespace uhd { namespace usrp { namespace gpio_atr {

class gpio_atr_3000_impl : public gpio_atr_3000{
public:
    gpio_atr_3000_impl(
        wb_iface::sptr iface,
        const wb_iface::wb_addr_type base,
        const wb_iface::wb_addr_type rb_addr = 0xFFFFFFFF
    ):
        _iface(iface), _rb_addr(rb_addr),
        _atr_idle_reg(REG_ATR_IDLE_OFFSET),
        _atr_rx_reg(REG_ATR_RX_OFFSET),
        _atr_tx_reg(REG_ATR_TX_OFFSET),
        _atr_fdx_reg(REG_ATR_FDX_OFFSET),
        _ddr_reg(REG_DDR_OFFSET),
        _atr_disable_reg(REG_ATR_DISABLE_OFFSET)
    {
        _atr_idle_reg.initialize(*_iface, true);
        _atr_rx_reg.initialize(*_iface, true);
        _atr_tx_reg.initialize(*_iface, true);
        _atr_fdx_reg.initialize(*_iface, true);
        _ddr_reg.initialize(*_iface, true);
        _atr_disable_reg.initialize(*_iface, true);
    }

    virtual void set_atr_mode(const gpio_atr_mode_t mode, const boost::uint32_t mask)
    {
        _atr_disable_reg.set_with_mask((mode==MODE_ATR)?0:0xFFFFFFFF, mask);
        _atr_disable_reg.flush();
    }

    virtual void set_gpio_ddr(const gpio_ddr_t dir, const boost::uint32_t mask)
    {
        _ddr_reg.set_with_mask((dir==DDR_INPUT)?0:0xFFFFFFFF, mask);
        _ddr_reg.flush();
    }

    virtual void set_atr_reg(const gpio_atr_reg_t atr, const boost::uint32_t value, const boost::uint32_t mask)
    {
        masked_reg_t* reg = NULL;
        switch (atr) {
            case ATR_REG_IDLE:          reg = &_atr_idle_reg; break;
            case ATR_REG_RX_ONLY:       reg = &_atr_rx_reg;   break;
            case ATR_REG_TX_ONLY:       reg = &_atr_tx_reg;   break;
            case ATR_REG_FULL_DUPLEX:   reg = &_atr_fdx_reg;  break;
            default:                    reg = &_atr_idle_reg; break;
        }
        reg->set_with_mask(value, mask);
        reg->flush();
    }

    virtual boost::uint32_t read_gpio()
    {
        if (_rb_addr != 0xFFFFFFFF) {
            return _iface->peek32(_rb_addr);
        } else {
            throw uhd::runtime_error("read_gpio not supported for write-only interface.");
        }
    }

    virtual void set_gpio_attr(const gpio_attr_t attr, const boost::uint32_t value)
    {
        switch (attr)
        {
        case GPIO_CTRL:
            set_atr_mode(MODE_ATR, value);
            set_atr_mode(MODE_GPIO, ~value);
            break;
        case GPIO_DDR:
            set_gpio_ddr(DDR_OUTPUT, value);
            set_gpio_ddr(DDR_INPUT, ~value);
            break;
        case GPIO_OUT:
            set_atr_reg(ATR_REG_IDLE, value, _atr_disable_reg.get(masked_reg_t::REGISTER));
            break;
        case GPIO_ATR_0X:
            set_atr_reg(ATR_REG_IDLE, value, ~_atr_disable_reg.get(masked_reg_t::REGISTER));
            break;
        case GPIO_ATR_RX:
            set_atr_reg(ATR_REG_RX_ONLY, value, ~_atr_disable_reg.get(masked_reg_t::REGISTER));
            break;
        case GPIO_ATR_TX:
            set_atr_reg(ATR_REG_TX_ONLY, value, ~_atr_disable_reg.get(masked_reg_t::REGISTER));
            break;
        case GPIO_ATR_XX:
            set_atr_reg(ATR_REG_FULL_DUPLEX, value, ~_atr_disable_reg.get(masked_reg_t::REGISTER));
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

protected:
    class masked_reg_t : public uhd::soft_reg32_wo_t {
    public:
        masked_reg_t(const wb_iface::wb_addr_type offset): uhd::soft_reg32_wo_t(offset) {
            set(REGISTER, 0);
        }

        inline void set_with_mask(const boost::uint32_t value, const boost::uint32_t mask) {
            set(REGISTER, (value&mask)|(get(REGISTER)&(~mask)));
        }
    };

    wb_iface::sptr          _iface;
    wb_iface::wb_addr_type  _rb_addr;
    masked_reg_t            _atr_idle_reg;
    masked_reg_t            _atr_rx_reg;
    masked_reg_t            _atr_tx_reg;
    masked_reg_t            _atr_fdx_reg;
    masked_reg_t            _ddr_reg;
    masked_reg_t            _atr_disable_reg;
};

gpio_atr_3000::sptr gpio_atr_3000::make(wb_iface::sptr iface, const size_t base, const size_t rb_addr) {
    return sptr(new gpio_atr_3000_impl(iface, base, rb_addr));
}

gpio_atr_3000::sptr gpio_atr_3000::make_write_only(wb_iface::sptr iface, const size_t base) {
    gpio_atr_3000::sptr gpio_iface(new gpio_atr_3000_impl(iface, base));
    gpio_iface->set_gpio_ddr(DDR_OUTPUT, 0xFFFFFFFF);
    return gpio_iface;
}

//-------------------------------------------------------------
// db_gpio_atr_3000
//-------------------------------------------------------------

class db_gpio_atr_3000_impl : public gpio_atr_3000_impl, public db_gpio_atr_3000 {
public:
    db_gpio_atr_3000_impl(wb_iface::sptr iface, const wb_iface::wb_addr_type base, const wb_iface::wb_addr_type rb_addr):
        gpio_atr_3000_impl(iface, base, rb_addr) { /* NOP */ }

    inline void set_pin_ctrl(const db_unit_t unit, const boost::uint16_t value)
    {
        gpio_atr_3000_impl::set_atr_mode(MODE_ATR,  compute_mask(unit, value));
        gpio_atr_3000_impl::set_atr_mode(MODE_GPIO, compute_mask(unit, ~value));
    }

    inline void set_gpio_ddr(const db_unit_t unit, const boost::uint16_t value)
    {
        gpio_atr_3000_impl::set_gpio_ddr(DDR_OUTPUT, compute_mask(unit, value));
        gpio_atr_3000_impl::set_gpio_ddr(DDR_INPUT,  compute_mask(unit, ~value));
    }

    inline void set_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr, const boost::uint16_t value)
    {
        gpio_atr_3000_impl::set_atr_reg(atr,
            static_cast<boost::uint32_t>(value) << compute_shift(unit),
            compute_mask(unit, ~(_atr_disable_reg.get(masked_reg_t::REGISTER))));
    }

    inline void set_gpio_out(const db_unit_t unit, const boost::uint16_t value)
    {
        gpio_atr_3000_impl::set_atr_reg(ATR_REG_IDLE,
            static_cast<boost::uint32_t>(value) << compute_shift(unit),
            compute_mask(unit, _atr_disable_reg.get(masked_reg_t::REGISTER)));
    }

    inline boost::uint16_t read_gpio(const db_unit_t unit)
    {
        return boost::uint16_t(gpio_atr_3000_impl::read_gpio() >> compute_shift(unit));
    }

private:
    inline boost::uint32_t compute_shift(const db_unit_t unit) {
        return (unit == dboard_iface::UNIT_RX) ? 0 : 16;
    }

    inline boost::uint32_t compute_mask(const db_unit_t unit, const boost::uint16_t mask) {
        return static_cast<boost::uint32_t>(mask) << (compute_shift(unit));
    }
};

db_gpio_atr_3000::sptr db_gpio_atr_3000::make(wb_iface::sptr iface, const size_t base, const size_t rb_addr) {
    return sptr(new db_gpio_atr_3000_impl(iface, base, rb_addr));
}

}}}
