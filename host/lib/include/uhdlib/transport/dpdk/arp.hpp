//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk/service_queue.hpp>
#include <rte_arp.h>

namespace uhd { namespace transport { namespace dpdk {

struct arp_request
{
    struct rte_ether_addr tha;
    port_id_t port;
    rte_ipv4_addr tpa;
};

struct arp_entry
{
    struct rte_ether_addr mac_addr;
    std::vector<wait_req*> reqs;
};

}}} /* namespace uhd::transport::dpdk */
