//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <unordered_map>

using namespace uhd;
using namespace usrp;

//-------------------------------------------------------------
// gpio_atr_3000
//-------------------------------------------------------------

namespace {
// Special RB addr value to indicate no readback
// This value is invalid as a real address because it is not a multiple of 4
static constexpr wb_iface::wb_addr_type READBACK_DISABLED = 0xFFFFFFFF;
}; // namespace

namespace uhd { namespace usrp { namespace gpio_atr {

bool gpio_atr_offsets::is_writeonly() const
{
    return readback == READBACK_DISABLED;
}

gpio_atr_offsets gpio_atr_offsets::make_default(const uhd::wb_iface::wb_addr_type base,
    const uhd::wb_iface::wb_addr_type rb_addr,
    const size_t stride)
{
    gpio_atr_offsets offsets{
        base, // Idle
        static_cast<uhd::wb_iface::wb_addr_type>(base + stride), // RX
        static_cast<uhd::wb_iface::wb_addr_type>(base + stride * 2), // TX
        static_cast<uhd::wb_iface::wb_addr_type>(base + stride * 3), // Full Duplex
        static_cast<uhd::wb_iface::wb_addr_type>(base + stride * 4), // DDR
        static_cast<uhd::wb_iface::wb_addr_type>(base + stride * 5), // Disabled
        rb_addr, // Readback
    };
    return offsets;
}

gpio_atr_offsets gpio_atr_offsets::make_write_only(
    const uhd::wb_iface::wb_addr_type base, const size_t stride)
{
    return make_default(base, READBACK_DISABLED, stride);
}

class gpio_atr_3000_impl : public gpio_atr_3000
{
public:
    gpio_atr_3000_impl(wb_iface::sptr iface, const gpio_atr_offsets registers)
        : _iface(iface)
        , _rb_addr(registers.readback)
        , _atr_idle_reg(registers.idle, _atr_disable_reg)
        , _atr_rx_reg(registers.rx)
        , _atr_tx_reg(registers.tx)
        , _atr_fdx_reg(registers.duplex)
        , _ddr_reg(registers.ddr)
        , _atr_disable_reg(registers.disable)
    {
        _atr_idle_reg.initialize(*_iface, true);
        _atr_rx_reg.initialize(*_iface, true);
        _atr_tx_reg.initialize(*_iface, true);
        _atr_fdx_reg.initialize(*_iface, true);
        _ddr_reg.initialize(*_iface, true);
        _atr_disable_reg.initialize(*_iface, true);
        for (const auto& attr : gpio_attr_map) {
            if (attr.first == usrp::gpio_atr::GPIO_SRC
                || attr.first == usrp::gpio_atr::GPIO_READBACK) {
                // Don't set the SRC and READBACK, they're handled elsewhere.
                continue;
            }
            _attr_reg_state.emplace(attr.first, 0);
        }
    }

    void set_atr_mode(const gpio_atr_mode_t mode, const uint32_t mask) override
    {
        // Each bit in the "ATR Disable" register determines whether the respective bit in
        // the GPIO output bus is driven by the ATR engine or a static register. For each
        // bit position, a 1 means that the bit is static and 0 means that the bit is
        // driven by the ATR state machine. This setting will only get applied to all bits
        // in the "mask" that are 1. All other bits will retain their old value.
        auto value = (mode == MODE_ATR) ? ~MASK_SET_ALL : MASK_SET_ALL;
        _atr_disable_reg.set_with_mask(value, mask);
        _atr_disable_reg.flush();
        // The attr state is inverted from _atr_disable_reg. In _atr_disable_reg,
        // 1 == disable, whereas in CTRL, 1 means enable ATR.
        _update_attr_state(GPIO_CTRL, ~value, mask);
    }

    void set_gpio_ddr(const gpio_ddr_t dir, const uint32_t mask) override
    {
        // Each bit in the "DDR" register determines whether the respective bit in the
        // GPIO bus is an input or an output. For each bit position, a 1 means that the
        // bit is an output and 0 means that the bit is an input. This setting will only
        // get applied to all bits in the "mask" that are 1. All other bits will retain
        // their old value.
        auto value = (dir == DDR_INPUT) ? ~MASK_SET_ALL : MASK_SET_ALL;
        _ddr_reg.set_with_mask(value, mask);
        _ddr_reg.flush();
        _update_attr_state(GPIO_DDR, value, mask);
    }

    void set_atr_reg(const gpio_atr_reg_t atr,
        const uint32_t value,
        const uint32_t mask = MASK_SET_ALL) override
    {
        // Set the value of the specified ATR register. For bits with ATR Disable set to
        // 1, the IDLE register will hold the output state This setting will only get
        // applied to all bits in the "mask" that are 1. All other bits will retain their
        // old value.
        masked_reg_t* reg = NULL;
        gpio_attr_t attr;
        switch (atr) {
            case ATR_REG_IDLE:
                reg  = &_atr_idle_reg;
                attr = GPIO_ATR_0X;

                break;
            case ATR_REG_RX_ONLY:
                reg  = &_atr_rx_reg;
                attr = GPIO_ATR_RX;

                break;
            case ATR_REG_TX_ONLY:
                reg  = &_atr_tx_reg;
                attr = GPIO_ATR_TX;

                break;
            case ATR_REG_FULL_DUPLEX:
                reg  = &_atr_fdx_reg;
                attr = GPIO_ATR_XX;

                break;
            default:
                reg  = &_atr_idle_reg;
                attr = GPIO_ATR_0X;
                break;
        }
        // For protection we only write to bits that have the mode ATR by masking the user
        // specified "mask" with ~atr_disable.
        reg->set_with_mask(value, mask);
        reg->flush();
        _update_attr_state(attr, value, mask);
    }

    void set_gpio_out(const uint32_t value, const uint32_t mask = MASK_SET_ALL) override
    {
        // Set the value of the specified GPIO output register.
        // This setting will only get applied to all bits in the "mask" that are 1. All
        // other bits will retain their old value.

        // For protection we only write to bits that have the mode GPIO by masking the
        // user specified "mask" with atr_disable.
        _atr_idle_reg.set_gpio_out_with_mask(value, mask);
        _atr_idle_reg.flush();
        _update_attr_state(GPIO_OUT, value, mask);
    }

    uint32_t read_gpio() override
    {
        // Read the state of the GPIO pins
        // If a pin is configured as an input, reads the actual value of the pin
        // If a pin is configured as an output, reads the last value written to the pin
        if (_rb_addr != READBACK_DISABLED) {
            return _iface->peek32(_rb_addr);
        } else {
            throw uhd::runtime_error("read_gpio not supported for write-only interface.");
        }
    }

    uint32_t get_attr_reg(const gpio_attr_t attr) override
    {
        if (attr == GPIO_SRC) {
            throw uhd::runtime_error("Can't get GPIO source by GPIO ATR interface.");
        }
        if (attr == GPIO_READBACK) {
            return read_gpio();
        }
        if (!_attr_reg_state.count(attr)) {
            throw uhd::runtime_error("Invalid GPIO attr!");
        }
        // Return the cached value of the requested attr
        return _attr_reg_state.at(attr);
    }

    inline void set_gpio_attr(const gpio_attr_t attr, const uint32_t value) override
    {
        // An attribute based API to configure all settings for the GPIO bus in one
        // function call. This API does not have a mask so it configures all bits at the
        // same time.
        switch (attr) {
            case GPIO_SRC:
                throw uhd::runtime_error("Can't set GPIO source by GPIO ATR interface.");
            case GPIO_CTRL:
                set_atr_mode(
                    MODE_ATR, value); // Configure mode=ATR for all bits that are set
                set_atr_mode(
                    MODE_GPIO, ~value); // Configure mode=GPIO for all bits that are unset
                break;
            case GPIO_DDR:
                set_gpio_ddr(
                    DDR_OUTPUT, value); // Configure as output for all bits that are set
                set_gpio_ddr(
                    DDR_INPUT, ~value); // Configure as input for all bits that are unset
                break;
            case GPIO_OUT:
                // Only set bits that are driven statically
                set_gpio_out(value);
                break;
            case GPIO_ATR_0X:
                // Only set bits that are driven by the ATR engine
                set_atr_reg(ATR_REG_IDLE, value);
                break;
            case GPIO_ATR_RX:
                // Only set bits that are driven by the ATR engine
                set_atr_reg(ATR_REG_RX_ONLY, value);
                break;
            case GPIO_ATR_TX:
                // Only set bits that are driven by the ATR engine
                set_atr_reg(ATR_REG_TX_ONLY, value);
                break;
            case GPIO_ATR_XX:
                // Only set bits that are driven by the ATR engine
                set_atr_reg(ATR_REG_FULL_DUPLEX, value);
                break;
            case GPIO_READBACK:
                // This is readonly register, ignore on set.
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

protected:
    class masked_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        masked_reg_t(const wb_iface::wb_addr_type offset) : uhd::soft_reg32_wo_t(offset)
        {
            uhd::soft_reg32_wo_t::set(REGISTER, 0);
        }

        virtual void set_with_mask(const uint32_t value, const uint32_t mask)
        {
            uhd::soft_reg32_wo_t::set(REGISTER,
                (value & mask) | (uhd::soft_reg32_wo_t::get(REGISTER) & (~mask)));
        }

        virtual uint32_t get()
        {
            return uhd::soft_reg32_wo_t::get(uhd::soft_reg32_wo_t::REGISTER);
        }

        void flush() override
        {
            uhd::soft_reg32_wo_t::flush();
        }
    };

    class atr_idle_reg_t : public masked_reg_t
    {
    public:
        atr_idle_reg_t(const wb_iface::wb_addr_type offset, masked_reg_t& atr_disable_reg)
            : masked_reg_t(offset)
            , _atr_idle_cache(0)
            , _gpio_out_cache(0)
            , _atr_disable_reg(atr_disable_reg)
        {
        }

        void set_with_mask(const uint32_t value, const uint32_t mask) override
        {
            _atr_idle_cache = (value & mask) | (_atr_idle_cache & (~mask));
        }

        uint32_t get() override
        {
            return _atr_idle_cache;
        }

        void set_gpio_out_with_mask(const uint32_t value, const uint32_t mask)
        {
            _gpio_out_cache = (value & mask) | (_gpio_out_cache & (~mask));
        }

        virtual uint32_t get_gpio_out()
        {
            return _gpio_out_cache;
        }

        void flush() override
        {
            set(REGISTER,
                (_atr_idle_cache & (~_atr_disable_reg.get()))
                    | (_gpio_out_cache & _atr_disable_reg.get()));
            masked_reg_t::flush();
        }

    private:
        uint32_t _atr_idle_cache;
        uint32_t _gpio_out_cache;
        masked_reg_t& _atr_disable_reg;
    };

    std::unordered_map<gpio_attr_t, uint32_t, std::hash<size_t>> _attr_reg_state;
    wb_iface::sptr _iface;
    wb_iface::wb_addr_type _rb_addr;
    atr_idle_reg_t _atr_idle_reg;
    masked_reg_t _atr_rx_reg;
    masked_reg_t _atr_tx_reg;
    masked_reg_t _atr_fdx_reg;
    masked_reg_t _ddr_reg;
    masked_reg_t _atr_disable_reg;

    void _update_attr_state(
        const gpio_attr_t attr, const uint32_t val, const uint32_t mask)
    {
        _attr_reg_state[attr] = (_attr_reg_state.at(attr) & ~mask) | (val & mask);
    }
};

gpio_atr_3000::sptr gpio_atr_3000::make(wb_iface::sptr iface, gpio_atr_offsets registers)
{
    gpio_atr_3000::sptr gpio_iface =
        std::make_shared<gpio_atr_3000_impl>(iface, registers);
    if (registers.is_writeonly()) {
        gpio_iface->set_gpio_ddr(DDR_OUTPUT, MASK_SET_ALL);
    }
    gpio_iface->set_gpio_ddr(DDR_OUTPUT, MASK_SET_ALL);
    return gpio_iface;
}

//-------------------------------------------------------------
// db_gpio_atr_3000
//-------------------------------------------------------------

class db_gpio_atr_3000_impl : public gpio_atr_3000_impl, public db_gpio_atr_3000
{
public:
    db_gpio_atr_3000_impl(wb_iface::sptr iface, const gpio_atr_offsets registers)
        : gpio_atr_3000_impl(iface, registers)
    { /* NOP */
    }

    inline void set_pin_ctrl(
        const db_unit_t unit, const uint32_t value, const uint32_t mask) override
    {
        gpio_atr_3000_impl::set_atr_mode(MODE_ATR, compute_mask(unit, value & mask));
        gpio_atr_3000_impl::set_atr_mode(MODE_GPIO, compute_mask(unit, (~value) & mask));
    }

    inline uint32_t get_pin_ctrl(const db_unit_t unit) override
    {
        return (~_atr_disable_reg.get()) >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_gpio_ddr;
    inline void set_gpio_ddr(
        const db_unit_t unit, const uint32_t value, const uint32_t mask) override
    {
        gpio_atr_3000_impl::set_gpio_ddr(DDR_OUTPUT, compute_mask(unit, value & mask));
        gpio_atr_3000_impl::set_gpio_ddr(DDR_INPUT, compute_mask(unit, (~value) & mask));
    }

    inline uint32_t get_gpio_ddr(const db_unit_t unit) override
    {
        return _ddr_reg.get() >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_atr_reg;
    inline void set_atr_reg(const db_unit_t unit,
        const gpio_atr_reg_t atr,
        const uint32_t value,
        const uint32_t mask) override
    {
        gpio_atr_3000_impl::set_atr_reg(
            atr, value << compute_shift(unit), compute_mask(unit, mask));
    }

    inline uint32_t get_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr) override
    {
        masked_reg_t* reg = NULL;
        switch (atr) {
            case ATR_REG_IDLE:
                reg = &_atr_idle_reg;
                break;
            case ATR_REG_RX_ONLY:
                reg = &_atr_rx_reg;
                break;
            case ATR_REG_TX_ONLY:
                reg = &_atr_tx_reg;
                break;
            case ATR_REG_FULL_DUPLEX:
                reg = &_atr_fdx_reg;
                break;
            default:
                reg = &_atr_idle_reg;
                break;
        }
        return (reg->get() & compute_mask(unit, MASK_SET_ALL)) >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::set_gpio_out;
    inline void set_gpio_out(
        const db_unit_t unit, const uint32_t value, const uint32_t mask) override
    {
        gpio_atr_3000_impl::set_gpio_out(
            static_cast<uint32_t>(value) << compute_shift(unit),
            compute_mask(unit, mask));
    }

    inline uint32_t get_gpio_out(const db_unit_t unit) override
    {
        return (_atr_idle_reg.get_gpio_out() & compute_mask(unit, MASK_SET_ALL))
               >> compute_shift(unit);
    }

    using gpio_atr_3000_impl::read_gpio;
    inline uint32_t read_gpio(const db_unit_t unit) override
    {
        return (gpio_atr_3000_impl::read_gpio() & compute_mask(unit, MASK_SET_ALL))
               >> compute_shift(unit);
    }

private:
    inline uint32_t compute_shift(const db_unit_t unit)
    {
        switch (unit) {
            case dboard_iface::UNIT_RX:
                return 0;
            case dboard_iface::UNIT_TX:
                return 16;
            default:
                return 0;
        }
    }

    inline uint32_t compute_mask(const db_unit_t unit, const uint32_t mask)
    {
        uint32_t tmp_mask = (unit == dboard_iface::UNIT_BOTH) ? mask : (mask & 0xFFFF);
        return tmp_mask << (compute_shift(unit));
    }
};

db_gpio_atr_3000::sptr db_gpio_atr_3000::make(
    wb_iface::sptr iface, gpio_atr_offsets registers)
{
    return std::make_shared<db_gpio_atr_3000_impl>(iface, registers);
}

}}} // namespace uhd::usrp::gpio_atr
