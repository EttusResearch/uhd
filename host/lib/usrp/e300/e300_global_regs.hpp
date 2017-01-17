//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_E300_GLOBAL_REGS_HPP
#define INCLUDED_E300_GLOBAL_REGS_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/transport/zero_copy.hpp>

namespace uhd { namespace usrp { namespace e300 {

struct global_regs_transaction_t {
    global_regs_transaction_t(): is_poke(0), addr(0), data(0), pad(0) {}
    uint32_t is_poke;
    uint32_t addr;
    uint32_t data;
    uint32_t pad;
};

class global_regs : boost::noncopyable, public virtual uhd::wb_iface
{
public:
    typedef boost::shared_ptr<global_regs> sptr;

    static sptr make(const size_t ctrl_base);
    static sptr make(uhd::transport::zero_copy_if::sptr xport);

    static const size_t SR_CORE_READBACK = 0;
    static const size_t SR_CORE_MISC     = 4;
    static const size_t SR_CORE_TEST     = 28;
    static const size_t SR_CORE_XB_LOCAL = 32;

    // leave some room for registers,
    // xbar starts with an offset of one
    // 1K page. A part of which is used for
    // DST_LOOKUP for DST_LOOKUP

    static const size_t SR_CORE_DST       = 1024;
    static const size_t SR_CORE_XBAR      = 2048;

    static const size_t RB32_CORE_MISC    = 1;
    static const size_t RB32_CORE_COMPAT  = 2;
    static const size_t RB32_CORE_GITHASH = 3;
    static const size_t RB32_CORE_PLL     = 4;
    static const size_t RB32_CORE_TEST    = 24;

    // PPS selection
    static const size_t PPS_GPS = 0;
    static const size_t PPS_INT = 2;
    static const size_t PPS_EXT = 3;
};

UHD_INLINE uint32_t XB_ADDR(const uint32_t addr)
{
    return global_regs::SR_CORE_XBAR + (addr << 2);
}

UHD_INLINE uint32_t DST_ADDR(const uint32_t addr)
{
    return global_regs::SR_CORE_DST + (addr << 2);
}

}}};

#endif /* INCLUDED_E300_GLOBAL_REGS_HPP */
