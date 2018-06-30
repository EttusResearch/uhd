//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>

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
        const wb_iface::wb_addr_type rb_addr = READBACK_DISABLED
    ):
        _iface(iface), _rb_addr(rb_addr),
        _atr_idle_reg(REG_ATR_IDLE_OFFSET, _atr_disable_reg),
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

    virtual void set_atr_mode(const gpio_atr_mode_t mode, const uint32_t mask)
    {
        //Each bit in the "ATR Disable" register determines whether the respective bit in the GPIO
        //output bus is driven by the ATR engine or a static register.
        //For each bit position, a 1 means that the bit is static and 0 means that the bit
        //is driven by the ATR state machine.
        //This setting will only get applied to all bits in the "mask" that are 1. All other
        //bits will retain their old value.
        _atr_disable_reg.set_with_mask((mode==MODE_ATR) ? ~MASK_SET_ALL : MASK_SET_ALL, mask);
        _atr_disable_reg.flush();
    }

    virtual void set_gpio_ddr(const gpio_ddr_t dir, const uint32_t mask)
    {
        //Each bit in the "DDR" register determines whether the respective bit in the GPIO
        //bus is an input or an output.
        //For each bit position, a 1 means that the bit is an output and 0 means that the bit
        //is an input.
        //This setting will only get applied to all bits in the "mask" that are 1. All other
        //bits will retain their old value.
        _ddr_reg.set_with_mask((dir==DDR_INPUT) ? ~MASK_SET_ALL : MASK_SET_ALL, mask);
        _ddr_reg.flush();
    }

    virtual void set_atr_reg(const gpio_atr_reg_t atr, const uint32_t value, const uint32_t mask = MASK_SET_ALL)
    {
        //Set the value of the specified ATR register. For bits with ATR Disable set to 1,
        //the IDLE register will hold the output state
        //This setting will only get applied to all bits in the "mask" that are 1. All other
        //bits will retain their old value.
        masked_reg_t* reg = NULL;
        switch (atr) {
            case ATR_REG_IDLE:          reg = &_atr_idle_reg; break;
            case ATR_REG_RX_ONLY:       reg = &_atr_rx_reg;   break;
            case ATR_REG_TX_ONLY:       reg = &_atr_tx_reg;   break;
            case ATR_REG_FULL_DUPLEX:   reg = &_atr_fdx_reg;  break;
            default:                    reg = &_atr_idle_reg; break;
        }
        //For protection we only write to bits that have the mode ATR by masking the user
        //specified "mask" with ~atr_disable.
        reg->set_with_mask(value, mask);
        reg->flush();
    }

    virtual void set_gpio_out(const uint32_t value, const uint32_t mask = MASK_SET_ALL) {
        //Set the value of the specified GPIO output register.
        //This setting will only get applied to all bits in the "mask" that are 1. All other
        //bits will retain their old value.

        //For protection we only write to bits that have the mode GPIO by masking the user
        //specified "mask" with atr_disable.
        _atr_idle_reg.set_gpio_out_with_mask(value, mask);
        _atr_idle_reg.flush();
    }

    virtual uint32_t read_gpio()
    {
        //Read the state of the GPIO pins
        //If a pin is configured as an input, reads the actual value of the pin
        //If a pin is configured as an output, reads the last value written to the pin
        if (_rb_addr != READBACK_DISABLED) {
            return _iface->peek32(_rb_addr);
        } else {
            throw uhd::runtime_error("read_gpio not supported for write-only interface.");
        }
    }

    inline virtual void set_gpio_attr(const gpio_attr_t attr, const uint32_t value)
    {
        //An attribute based API to configure all settings for the GPIO bus in one function
        //call. This API does not have a mask so it configures all bits at the same time.
        switch (attr)
        {
        case GPIO_SRC:
            throw uhd::runtime_error("Can't set GPIO source by GPIO ATR interface.");
        case GPIO_CTRL:
            set_atr_mode(MODE_ATR, value);   //Configure mode=ATR for all bits that are set
            set_atr_mode(MODE_GPIO, ~value); //Configure mode=GPIO for all bits that are unset
            break;
        case GPIO_DDR:
            set_gpio_ddr(DDR_OUTPUT, value); //Configure as output for all bits that are set
            set_gpio_ddr(DDR_INPUT, ~value); //Configure as input for all bits that are unset
            break;
        case GPIO_OUT:
            //Only set bits that are driven statically
            set_gpio_out(value);
            break;
        case GPIO_ATR_0X:
            //Only set bits that are driven by the ATR engine
            set_atr_reg(ATR_REG_IDLE, value);
            break;
        case GPIO_ATR_RX:
            //Only set bits that are driven by the ATR engine
            set_atr_reg(ATR_REG_RX_ONLY, value);
            break;
        case GPIO_ATR_TX:
            //Only set bits that are driven by the ATR engine
            set_atr_reg(ATR_REG_TX_ONLY, value);
            break;
        case GPIO_ATR_XX:
            //Only set bits that are driven by the ATR engine
            set_atr_reg(ATR_REG_FULL_DUPLEX, value);
            break;
        case GPIO_READBACK:
            //This is readonly register, ignore on set. 
            break;  
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

protected:
    //Special RB addr value to indicate no readback
    //This value is invalid as a real address because it is not a multiple of 4
    static const wb_iface::wb_addr_type READBACK_DISABLED = 0xFFFFFFFF;

    class masked_reg_t : public uhd::soft_reg32_wo_t {
    public:
        masked_reg_t(const wb_iface::wb_addr_type offset): uhd::soft_reg32_wo_t(offset) {
            uhd::soft_reg32_wo_t::set(REGISTER, 0);
        }

        virtual void set_with_mask(const uint32_t value, const uint32_t mask) {
            uhd::soft_reg32_wo_t::set(REGISTER,
                (value&mask)|(uhd::soft_reg32_wo_t::get(REGISTER)&(~mask)));
        }

        virtual uint32_t get() {
            return uhd::soft_reg32_wo_t::get(uhd::soft_reg32_wo_t::REGISTER);
        }

        virtual void flush() {
            uhd::soft_reg32_wo_t::flush();
        }
    };

    class atr_idle_reg_t : public masked_reg_t {
    public:
        atr_idle_reg_t(const wb_iface::wb_addr_type offset, masked_reg_t& atr_disable_reg):
            masked_reg_t(offset),
            _atr_idle_cache(0), _gpio_out_cache(0),
            _atr_disable_reg(atr_disable_reg)
        { }

        virtual void set_with_mask(const uint32_t value, const uint32_t mask) {
            _atr_idle_cache = (value&mask)|(_atr_idle_cache&(~mask));
        }

        virtual uint32_t get() {
            return _atr_idle_cache;
        }

        void set_gpio_out_with_mask(const uint32_t value, const uint32_t mask) {
            _gpio_out_cache = (value&mask)|(_gpio_out_cache&(~mask));
        }

        virtual uint32_t get_gpio_out() {
            return _gpio_out_cache;
        }

        virtual void flush() {
            set(REGISTER,
                (_atr_idle_cache & (~_atr_disable_reg.get())) |
                (_gpio_out_cache & _atr_disable_reg.get())
            );
            masked_reg_t::flush();
        }

    private:
        uint32_t _atr_idle_cache;
        uint32_t _gpio_out_cache;
        masked_reg_t&   _atr_disable_reg;
    };

    wb_iface::sptr          _iface;
    wb_iface::wb_addr_type  _rb_addr;
    atr_idle_reg_t          _atr_idle_reg;
    masked_reg_t            _atr_rx_reg;
    masked_reg_t            _atr_tx_reg;
    masked_reg_t            _atr_fdx_reg;
    masked_reg_t            _ddr_reg;
    masked_reg_t            _atr_disable_reg;
};

gpio_atr_3000::sptr gpio_atr_3000::make(
    wb_iface::sptr iface, const wb_iface::wb_addr_type base, const wb_iface::wb_addr_type rb_addr
) {
    return sptr(new gpio_atr_3000_impl(iface, base, rb_addr));
}

gpio_atr_3000::sptr gpio_atr_3000::make_write_only(
    wb_iface::sptr iface, const wb_iface::wb_addr_type base
) {
    gpio_atr_3000::sptr gpio_iface(new gpio_atr_3000_impl(iface, base));
    gpio_iface->set_gpio_ddr(DDR_OUTPUT, MASK_SET_ALL);
    return gpio_iface;
}

//-------------------------------------------------------------
// db_gpio_atr_3000
//-------------------------------------------------------------

class db_gpio_atr_3000_impl : public gpio_atr_3000_impl, public db_gpio_atr_3000 {
public:
    db_gpio_atr_3000_impl(wb_iface::sptr iface, const wb_iface::wb_addr_type base, const wb_iface::wb_addr_type rb_addr):
        gpio_atr_3000_impl(iface, base, rb_addr) { /* NOP */ }

    inline void set_pin_ctrl(const db_unit_t unit, const uint32_t value, const uint32_t mask)
    {
        gpio_atr_3000_impl::set_atr_mode(MODE_ATR,  compute_mask(unit, value&mask));
        gpio_atr_3000_impl::set_atr_mode(MODE_GPIO, compute_mask(unit, (~value)&mask));
    }

    inline uint32_t get_pin_ctrl(const db_unit_t unit)
    {
        return (~_atr_disable_reg.get()) >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_gpio_ddr;
    inline void set_gpio_ddr(const db_unit_t unit, const uint32_t value, const uint32_t mask)
    {
        gpio_atr_3000_impl::set_gpio_ddr(DDR_OUTPUT, compute_mask(unit, value&mask));
        gpio_atr_3000_impl::set_gpio_ddr(DDR_INPUT,  compute_mask(unit, (~value)&mask));
    }

    inline uint32_t get_gpio_ddr(const db_unit_t unit)
    {
        return _ddr_reg.get() >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_atr_reg;
    inline void set_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr, const uint32_t value, const uint32_t mask)
    {
        gpio_atr_3000_impl::set_atr_reg(atr, value << compute_shift(unit), compute_mask(unit, mask));
    }

    inline uint32_t get_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr)
    {
        masked_reg_t* reg = NULL;
        switch (atr) {
            case ATR_REG_IDLE:          reg = &_atr_idle_reg; break;
            case ATR_REG_RX_ONLY:       reg = &_atr_rx_reg;   break;
            case ATR_REG_TX_ONLY:       reg = &_atr_tx_reg;   break;
            case ATR_REG_FULL_DUPLEX:   reg = &_atr_fdx_reg;  break;
            default:                    reg = &_atr_idle_reg; break;
        }
        return (reg->get() & compute_mask(unit, MASK_SET_ALL)) >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_gpio_out;
    inline void set_gpio_out(const db_unit_t unit, const uint32_t value, const uint32_t mask)
    {
        gpio_atr_3000_impl::set_gpio_out(
            static_cast<uint32_t>(value) << compute_shift(unit),
            compute_mask(unit, mask));
    }

    inline uint32_t get_gpio_out(const db_unit_t unit)
    {
        return (_atr_idle_reg.get_gpio_out() & compute_mask(unit, MASK_SET_ALL)) >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::read_gpio;
    inline uint32_t read_gpio(const db_unit_t unit)
    {
        return (gpio_atr_3000_impl::read_gpio() & compute_mask(unit, MASK_SET_ALL)) >> compute_shift(unit);
    }

private:
    inline uint32_t compute_shift(const db_unit_t unit) {
        switch (unit) {
        case dboard_iface::UNIT_RX: return 0;
        case dboard_iface::UNIT_TX: return 16;
        default:                    return 0;
        }
    }

    inline uint32_t compute_mask(const db_unit_t unit, const uint32_t mask) {
        uint32_t tmp_mask = (unit == dboard_iface::UNIT_BOTH) ? mask : (mask & 0xFFFF);
        return tmp_mask << (compute_shift(unit));
    }
};

db_gpio_atr_3000::sptr db_gpio_atr_3000::make(
    wb_iface::sptr iface, const wb_iface::wb_addr_type base, const wb_iface::wb_addr_type rb_addr
) {
    return sptr(new db_gpio_atr_3000_impl(iface, base, rb_addr));
}

}}}
