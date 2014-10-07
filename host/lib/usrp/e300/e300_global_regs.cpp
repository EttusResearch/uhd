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

#include "e300_global_regs.hpp"

#include <boost/cstdint.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <cstring>
#include <iostream>

namespace uhd { namespace usrp { namespace e300 {

class global_regs_local_impl : public global_regs
{
public:
    global_regs_local_impl(const size_t ctrl_base) : _ctrl_base(ctrl_base)
    {
    }

    virtual ~global_regs_local_impl(void)
    {
    }

    boost::uint32_t peek32(const uhd::wb_iface::wb_addr_type addr)
    {
        // setup readback register
        _poke32(_ctrl_base + global_regs::SR_CORE_READBACK, addr);
        return _peek32(_ctrl_base);
    }

    void poke32(const uhd::wb_iface::wb_addr_type addr, const boost::uint32_t data)
    {
        _poke32(_ctrl_base + static_cast<size_t>(addr), data);
    }


private:
    const size_t _ctrl_base;

    UHD_INLINE void _poke32(const boost::uint32_t addr, const boost::uint32_t data)
    {
        volatile boost::uint32_t *p = reinterpret_cast<boost::uint32_t *>(addr);
        *p = data;
    }

    UHD_INLINE boost::uint32_t _peek32(const boost::uint32_t addr)
    {
        volatile const boost::uint32_t *p = reinterpret_cast<const boost::uint32_t *>(addr);
        return *p;
    }
};

global_regs::sptr global_regs::make(const size_t ctrl_base)
{
    return sptr(new global_regs_local_impl(ctrl_base));
}

class global_regs_zc_impl : public global_regs
{
public:
    global_regs_zc_impl(uhd::transport::zero_copy_if::sptr xport) : _xport(xport)
    {
    }

    virtual ~global_regs_zc_impl(void)
    {
    }

    boost::uint32_t peek32(const uhd::wb_iface::wb_addr_type addr)
    {
        global_regs_transaction_t transaction;
        transaction.is_poke = uhd::htonx<boost::uint32_t>(0);
        transaction.addr    = uhd::htonx<boost::uint32_t>(
            static_cast<boost::uint32_t>(addr));
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < sizeof(transaction))
                throw std::runtime_error("global_regs_zc_impl send timeout");
            std::memcpy(buff->cast<void *>(), &transaction, sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff = _xport->get_recv_buff(10.0);
            if (not buff or buff->size() < sizeof(transaction))
                throw std::runtime_error("global_regs_zc_impl recv timeout");
            std::memcpy(&transaction, buff->cast<const void *>(), sizeof(transaction));
        }
        return uhd::ntohx<boost::uint32_t>(transaction.data);
    }

    void poke32(const uhd::wb_iface::wb_addr_type addr, const boost::uint32_t data)
    {
        global_regs_transaction_t transaction;
        transaction.is_poke = uhd::htonx<boost::uint32_t>(1);
        transaction.addr    = uhd::htonx<boost::uint32_t>(
            static_cast<boost::uint32_t>(addr));
        transaction.data    = uhd::htonx<boost::uint32_t>(data);
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < sizeof(transaction))
                throw uhd::runtime_error("global_regs_zc_impl send timeout");
            std::memcpy(buff->cast<void *>(), &transaction, sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
    }

private:
    uhd::transport::zero_copy_if::sptr _xport;
};

global_regs::sptr global_regs::make(uhd::transport::zero_copy_if::sptr xport)
{
    return sptr(new global_regs_zc_impl(xport));
}

}}};
