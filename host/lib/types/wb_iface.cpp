//
// Copyright 2013,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/wb_iface.hpp>
#include <uhd/exception.hpp>

using namespace uhd;

wb_iface::~wb_iface(void)
{
    //NOP
}

void wb_iface::poke64(const wb_iface::wb_addr_type, const uint64_t)
{
    throw uhd::not_implemented_error("poke64 not implemented");
}

uint64_t wb_iface::peek64(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek64 not implemented");
}

void wb_iface::poke32(const wb_iface::wb_addr_type, const uint32_t)
{
    throw uhd::not_implemented_error("poke32 not implemented");
}

uint32_t wb_iface::peek32(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek32 not implemented");
}

void wb_iface::poke16(const wb_iface::wb_addr_type, const uint16_t)
{
    throw uhd::not_implemented_error("poke16 not implemented");
}

uint16_t wb_iface::peek16(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek16 not implemented");
}
