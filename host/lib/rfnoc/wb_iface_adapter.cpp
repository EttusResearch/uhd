//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "wb_iface_adapter.hpp"

using namespace uhd::rfnoc;

wb_iface_adapter::wb_iface_adapter(
        const poke32_type &poke32_functor_,
        const peek32_type &peek32_functor_,
        const peek64_type &peek64_functor_,
        const gettime_type &gettime_functor_,
        const settime_type &settime_functor_
) : poke32_functor(poke32_functor_)
  , peek32_functor(peek32_functor_)
  , peek64_functor(peek64_functor_)
  , gettime_functor(gettime_functor_)
  , settime_functor(settime_functor_)
{
    // nop
}

wb_iface_adapter::wb_iface_adapter(
        const poke32_type &poke32_functor_,
        const peek32_type &peek32_functor_,
        const peek64_type &peek64_functor_
) : poke32_functor(poke32_functor_)
  , peek32_functor(peek32_functor_)
  , peek64_functor(peek64_functor_)
{
    // nop
}

void wb_iface_adapter::poke32(const wb_addr_type addr, const uint32_t data)
{
    poke32_functor(addr / 4, data); // FIXME remove the requirement for /4
}

uint32_t wb_iface_adapter::peek32(const wb_addr_type addr)
{
    return peek32_functor(addr);
}

uint64_t wb_iface_adapter::peek64(const wb_addr_type addr)
{
    return peek64_functor(addr);
}

uhd::time_spec_t wb_iface_adapter::get_time(void)
{
    return gettime_functor();
}

void wb_iface_adapter::set_time(const uhd::time_spec_t& t)
{
    settime_functor(t);
}
