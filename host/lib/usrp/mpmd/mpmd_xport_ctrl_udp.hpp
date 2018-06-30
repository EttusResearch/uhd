//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_ctrl_udp_HPP
#define INCLUDED_MPMD_XPORT_ctrl_udp_HPP

#include "mpmd_xport_ctrl_base.hpp"
#include <uhd/types/device_addr.hpp>
#include "../device3/device3_impl.hpp"

namespace uhd { namespace mpmd { namespace xport {

/*! UDP transport manager
 *
 * Opens UDP sockets
 */
class mpmd_xport_ctrl_udp : public mpmd_xport_ctrl_base
{
public:
    mpmd_xport_ctrl_udp(
        const uhd::device_addr_t& mb_args
    );

    both_xports_t make_transport(
        mpmd_xport_mgr::xport_info_t& xport_info,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args
    );

    bool is_valid(
        const mpmd_xport_mgr::xport_info_t& xport_info
    ) const;

    size_t get_mtu(
        const uhd::direction_t dir
    ) const;

private:
    const uhd::device_addr_t _mb_args;
    const uhd::dict<std::string, std::string> _recv_args;
    const uhd::dict<std::string, std::string> _send_args;
    //! A list of IP addresses we can connect our CHDR connections to
    const std::vector<std::string> _available_addrs;
    //! MTU
    size_t _mtu;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_ctrl_udp_HPP */
