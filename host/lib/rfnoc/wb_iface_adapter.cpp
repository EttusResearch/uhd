//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/rfnoc/wb_iface_adapter.hpp>
#include <uhd/rfnoc/constants.hpp>

using namespace uhd::rfnoc;

wb_iface_adapter::wb_iface_adapter(
        ctrl_iface::sptr iface,
        const gettickrate_type & gettickrate_functor_,
        const settime_type & settime_functor_,
        const gettime_type & gettime_functor_
) : _iface(iface)
  , gettickrate_functor(gettickrate_functor_)
  , settime_functor(settime_functor_)
  , gettime_functor(gettime_functor_)
{
    // nop
}

void wb_iface_adapter::poke32(const wb_addr_type addr, const uint32_t data)
{
    const uint64_t timestamp = gettime_functor().to_ticks(gettickrate_functor());
    _iface->send_cmd_pkt(addr / 4, data, false, timestamp);
}

uint32_t wb_iface_adapter::peek32(const wb_addr_type addr)
{
    const uint64_t reg_value = peek64(addr);
    return ((addr/4) & 0x1) ?
        uint32_t(reg_value >> 32) :
        uint32_t(reg_value & 0xffffffff);
}

uint64_t wb_iface_adapter::peek64(const wb_addr_type addr)
{
    const uint64_t timestamp = gettime_functor().to_ticks(gettickrate_functor());
    // TODO: Figure out if we should have a timestamp here
    _iface->send_cmd_pkt(SR_READBACK_ADDR, addr / 8, false, timestamp);
    return _iface->send_cmd_pkt(SR_READBACK, SR_READBACK_REG_USER, true, timestamp);
}

