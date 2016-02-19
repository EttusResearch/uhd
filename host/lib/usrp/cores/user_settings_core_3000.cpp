//
// Copyright 2012 Ettus Research LLC
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

#include "user_settings_core_3000.hpp"
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

    void poke64(const wb_addr_type offset, const boost::uint64_t value)
    {
        if (offset % sizeof(boost::uint64_t) != 0) throw uhd::value_error("poke64: Incorrect address alignment");
        poke32(offset, static_cast<boost::uint32_t>(value));
        poke32(offset + 4, static_cast<boost::uint32_t>(value >> 32));
    }

    boost::uint64_t peek64(const wb_addr_type offset)
    {
        if (offset % sizeof(boost::uint64_t) != 0) throw uhd::value_error("peek64: Incorrect address alignment");

        boost::unique_lock<boost::mutex> lock(_mutex);
        _iface->poke32(REG_USER_RB_ADDR, offset >> 3);  //Translate byte offset to 64-bit offset
        return _iface->peek64(_rb_reg_addr);
    }

    void poke32(const wb_addr_type offset, const boost::uint32_t value)
    {
        if (offset % sizeof(boost::uint32_t) != 0) throw uhd::value_error("poke32: Incorrect address alignment");

        boost::unique_lock<boost::mutex> lock(_mutex);
        _iface->poke32(REG_USER_SR_ADDR, offset >> 2);   //Translate byte offset to 64-bit offset
        _iface->poke32(REG_USER_SR_DATA, value);
    }

    boost::uint32_t peek32(const wb_addr_type offset)
    {
        if (offset % sizeof(boost::uint32_t) != 0) throw uhd::value_error("peek32: Incorrect address alignment");

        boost::uint64_t value = peek64((offset >> 3) << 3);
        if ((offset & 0x7) == 0) {
            return static_cast<boost::uint32_t>(value);
        } else {
            return static_cast<boost::uint32_t>(value >> 32);
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
