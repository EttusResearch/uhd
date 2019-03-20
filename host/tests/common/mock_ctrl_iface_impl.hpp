//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#ifndef INCLUDED_MOCK_CTRL_IFACE_IMPL_HPP
#define INCLUDED_MOCK_CTRL_IFACE_IMPL_HPP

#include <uhd/rfnoc/constants.hpp>
#include <uhdlib/rfnoc/ctrl_iface.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <queue>

class mock_ctrl_iface_impl : public uhd::rfnoc::ctrl_iface
{
    uint64_t send_cmd_pkt(const size_t addr,
        const size_t data,
        const bool readback      = false,
        const uint64_t timestamp = 0);

    void set_cmd_fifo_size(const size_t) {}
};
#endif /* INCLUDED_MOCK_CTRL_IFACE_IMPL_HPP */
