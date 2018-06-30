//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/cores/user_settings_core_3000.hpp>
#include <uhd/exception.hpp>
#include <boost/thread/thread.hpp>

using namespace uhd;

#define REG_USER_SR_ADDR   _sr_base_addr + 0
#define REG_USER_SR_DATA   _sr_base_addr + 4
#define REG_USER_RB_ADDR   _sr_base_addr + 8

class user_settings_core_3000_impl : public user_settings_core_3000 {
public:
    user_settings_core_3000_impl(
        wb_iface::sptr iface,
        const wb_addr_type sr_base_addr, const wb_addr_type rb_reg_addr):
        _iface(iface), _sr_base_addr(sr_base_addr), _rb_reg_addr(rb_reg_addr)
    {
    }

    void poke64(const wb_addr_type offset, const uint64_t value)
    {
        if (offset % sizeof(uint64_t) != 0) throw uhd::value_error("poke64: Incorrect address alignment");
        poke32(offset, static_cast<uint32_t>(value));
        poke32(offset + 4, static_cast<uint32_t>(value >> 32));
    }

    uint64_t peek64(const wb_addr_type offset)
    {
        if (offset % sizeof(uint64_t) != 0) throw uhd::value_error("peek64: Incorrect address alignment");

        boost::unique_lock<boost::mutex> lock(_mutex);
        _iface->poke32(REG_USER_RB_ADDR, offset >> 3);  //Translate byte offset to 64-bit offset
        return _iface->peek64(_rb_reg_addr);
    }

    void poke32(const wb_addr_type offset, const uint32_t value)
    {
        if (offset % sizeof(uint32_t) != 0) throw uhd::value_error("poke32: Incorrect address alignment");

        boost::unique_lock<boost::mutex> lock(_mutex);
        _iface->poke32(REG_USER_SR_ADDR, offset >> 2);   //Translate byte offset to 64-bit offset
        _iface->poke32(REG_USER_SR_DATA, value);
    }

    uint32_t peek32(const wb_addr_type offset)
    {
        if (offset % sizeof(uint32_t) != 0) throw uhd::value_error("peek32: Incorrect address alignment");

        uint64_t value = peek64((offset >> 3) << 3);
        if ((offset & 0x7) == 0) {
            return static_cast<uint32_t>(value);
        } else {
            return static_cast<uint32_t>(value >> 32);
        }
    }

private:
    wb_iface::sptr      _iface;
    const wb_addr_type  _sr_base_addr;
    const wb_addr_type  _rb_reg_addr;
    boost::mutex        _mutex;
};

wb_iface::sptr user_settings_core_3000::make(wb_iface::sptr iface,
    const wb_addr_type sr_base_addr, const wb_addr_type rb_reg_addr)
{
    return sptr(new user_settings_core_3000_impl(iface, sr_base_addr, rb_reg_addr));
}
