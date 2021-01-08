//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/types/wb_iface.hpp>
#include <memory>

//! Convenience macro to generate a reg_iface_adapter from within an RFNoC block
#define RFNOC_MAKE_WB_IFACE(BASE_OFFSET, CHAN)                                          \
    std::make_shared<reg_iface_adapter>([this]() -> register_iface& { return regs(); }, \
        [this, chan = CHAN]() { return get_command_time(chan); },                       \
        [this, chan = CHAN](                                                            \
            const uhd::time_spec_t& time) { set_command_time(time, chan); },            \
        BASE_OFFSET)

namespace uhd { namespace rfnoc {

/*! register_iface to wb_iface adapter
 *
 * From within a noc_block_base derivative, this call will work to create a
 * wb_iface:
 *
 * wb_iface::sptr ctrl = std::make_shared<reg_iface_adapter>(
 *     [this]() -> register_iface& { return regs(); }, offset);
 *
 * Or you use the macro:
 * wb_iface::sptr ctrl = RFNOC_MAKE_WB_IFACE(offset, chan);
 */
class UHD_API reg_iface_adapter : public uhd::timed_wb_iface
{
public:
    using regs_accessor_t = std::function<register_iface&(void)>;
    using time_accessor_t = std::function<uhd::time_spec_t(void)>;
    using time_setter_t   = std::function<void(const uhd::time_spec_t&)>;

    /*!
     * \param regs_accessor Function object to retrieve the register_iface
     *                      reference
     * \param time_accessor Function object to get the current command time
     * \param time_setter Function object to set the command time
     * \param base_offset Base offset for all peeks and pokes. If base_offset
     *                    is set to 0x80000, peek32(4) will poke 0x80004.
     */
    reg_iface_adapter(regs_accessor_t&& regs_accessor,
        time_accessor_t&& time_accessor,
        time_setter_t&& time_setter,
        const uint32_t base_offset = 0)
        : _regs_accessor(std::move(regs_accessor))
        , _time_accessor(std::move(time_accessor))
        , _time_setter(std::move(time_setter))
        , _base_offset(base_offset)
    {
        // nop
    }

    /*! Timeless constructor: All command times will be executed ASAP, setting
     * time does nothing
     *
     * \param regs_accessor Function object to retrieve the register_iface
     *                      reference
     * \param base_offset Base offset for all peeks and pokes. If base_offset
     *                    is set to 0x80000, peek32(4) will poke 0x80004.
     */
    reg_iface_adapter(regs_accessor_t&& regs_accessor, const uint32_t base_offset = 0)
        : _regs_accessor(std::move(regs_accessor))
        , _time_accessor([]() { return uhd::time_spec_t::ASAP; })
        , _time_setter([](const uhd::time_spec_t&) { /* nop */ })
        , _base_offset(base_offset)
    {
        // nop
    }

    void poke32(const uhd::wb_iface::wb_addr_type addr, const uint32_t data) override
    {
        _regs_accessor().poke32(_base_offset + addr, data, _time_accessor());
    }

    void poke64(const uhd::wb_iface::wb_addr_type addr, const uint64_t data) override
    {
        _regs_accessor().poke64(_base_offset + addr, data, _time_accessor());
    }

    uint32_t peek32(const uhd::wb_iface::wb_addr_type addr) override
    {
        return _regs_accessor().peek32(_base_offset + addr, _time_accessor());
    }

    uint64_t peek64(const uhd::wb_iface::wb_addr_type addr) override
    {
        return _regs_accessor().peek64(_base_offset + addr, _time_accessor());
    }

    uhd::time_spec_t get_time(void) override
    {
        return _time_accessor();
    }

    void set_time(const uhd::time_spec_t& t) override
    {
        _time_setter(t);
    }

private:
    regs_accessor_t _regs_accessor;
    time_accessor_t _time_accessor;
    time_setter_t _time_setter;
    uint32_t _base_offset;
};


}} /* namespace uhd::rfnoc */
