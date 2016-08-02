//
// Copyright 2016 Ettus Research LLC
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

void wb_iface_adapter::poke32(const wb_addr_type addr, const boost::uint32_t data)
{
    poke32_functor(addr / 4, data); // FIXME remove the requirement for /4
}

boost::uint32_t wb_iface_adapter::peek32(const wb_addr_type addr)
{
    return peek32_functor(addr);
}

boost::uint64_t wb_iface_adapter::peek64(const wb_addr_type addr)
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
