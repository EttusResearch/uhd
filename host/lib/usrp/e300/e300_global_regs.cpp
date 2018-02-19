//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e300_global_regs.hpp"

#include <stdint.h>
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

    uint32_t peek32(const uhd::wb_iface::wb_addr_type addr)
    {
        // setup readback register
        _poke32(_ctrl_base + global_regs::SR_CORE_READBACK, addr);
        return _peek32(_ctrl_base);
    }

    void poke32(const uhd::wb_iface::wb_addr_type addr, const uint32_t data)
    {
        _poke32(_ctrl_base + static_cast<size_t>(addr), data);
    }


private:
    const size_t _ctrl_base;

    UHD_INLINE void _poke32(const uint32_t addr, const uint32_t data)
    {
        volatile uint32_t *p = reinterpret_cast<uint32_t *>(addr);
        *p = data;
    }

    UHD_INLINE uint32_t _peek32(const uint32_t addr)
    {
        volatile const uint32_t *p = reinterpret_cast<const uint32_t *>(addr);
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

    uint32_t peek32(const uhd::wb_iface::wb_addr_type addr)
    {
        global_regs_transaction_t transaction;
        transaction.is_poke = uhd::htonx<uint32_t>(0);
        transaction.addr    = uhd::htonx<uint32_t>(
            static_cast<uint32_t>(addr));
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
        return uhd::ntohx<uint32_t>(transaction.data);
    }

    void poke32(const uhd::wb_iface::wb_addr_type addr, const uint32_t data)
    {
        global_regs_transaction_t transaction;
        transaction.is_poke = uhd::htonx<uint32_t>(1);
        transaction.addr    = uhd::htonx<uint32_t>(
            static_cast<uint32_t>(addr));
        transaction.data    = uhd::htonx<uint32_t>(data);
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
