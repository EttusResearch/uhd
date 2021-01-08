//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_CTRL_UDP_HPP
#define INCLUDED_MPMD_XPORT_CTRL_UDP_HPP

#include "mpmd_link_if_ctrl_base.hpp"
#include "mpmd_link_if_mgr.hpp"
#include <map>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>

namespace uhd { namespace mpmd { namespace xport {

/*! UDP link interface controller (for both kernel-based UDP and DPDK)
 *
 * Opens UDP sockets
 */
class mpmd_link_if_ctrl_udp : public mpmd_link_if_ctrl_base
{
public:
    struct udp_link_info_t
    {
        std::string udp_port;
        size_t link_rate;
        std::string link_type;
        size_t if_mtu;
    };

    using udp_link_info_map = std::map<std::string, udp_link_info_t>;

    mpmd_link_if_ctrl_udp(const uhd::device_addr_t& mb_args,
        const mpmd_link_if_mgr::xport_info_list_t& xport_info,
        const uhd::rfnoc::chdr_w_t chdr_w);

    size_t get_num_links() const override;
    uhd::transport::both_links_t get_link(const size_t link_idx,
        const uhd::transport::link_type_t link_type,
        const uhd::device_addr_t& link_args) override;
    size_t get_mtu(const uhd::direction_t) const override
    {
        return _mtu;
    }
    double get_link_rate(const size_t link_idx) const override;
    const uhd::rfnoc::chdr::chdr_packet_factory& get_packet_factory() const override;

private:
    const uhd::device_addr_t _mb_args;
    //!
    udp_link_info_map _udp_info;
    //! A list of IP addresses we can connect our CHDR connections to
    std::vector<std::string> _available_addrs;
    //! MTU
    size_t _mtu;
    const uhd::rfnoc::chdr::chdr_packet_factory _pkt_factory;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_CTRL_UDP_HPP */
