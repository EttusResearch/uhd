//
// Copyright 2013,2015 Ettus Research LLC
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

#include <uhd/types/wb_iface.hpp>
#include <uhd/exception.hpp>

using namespace uhd;

wb_iface::~wb_iface(void)
{
    //NOP
}

void wb_iface::poke64(const wb_iface::wb_addr_type, const boost::uint64_t)
{
    throw uhd::not_implemented_error("poke64 not implemented");
}

boost::uint64_t wb_iface::peek64(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek64 not implemented");
}

void wb_iface::poke32(const wb_iface::wb_addr_type, const boost::uint32_t)
{
    throw uhd::not_implemented_error("poke32 not implemented");
}

boost::uint32_t wb_iface::peek32(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek32 not implemented");
}

void wb_iface::poke16(const wb_iface::wb_addr_type, const boost::uint16_t)
{
    throw uhd::not_implemented_error("poke16 not implemented");
}

boost::uint16_t wb_iface::peek16(const wb_iface::wb_addr_type)
{
    throw uhd::not_implemented_error("peek16 not implemented");
}
