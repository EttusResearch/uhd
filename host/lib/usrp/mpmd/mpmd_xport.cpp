//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// make_transport logic for mpmd_impl. Note that mpmd_xport_mgr.* has most of
// the actual transport logic, this for transport-related APIs.

#include "mpmd_impl.hpp"
#include "mpmd_xport_mgr.hpp"

using namespace uhd;
using namespace uhd::mpmd;

uhd::device_addr_t mpmd_impl::get_rx_hints(size_t mb_index)
{
    return _mb.at(mb_index)->get_rx_hints();
}

uhd::device_addr_t mpmd_impl::get_tx_hints(size_t mb_index)
{
    return _mb.at(mb_index)->get_tx_hints();
}

size_t mpmd_impl::identify_mboard_by_xbar_addr(const size_t xbar_addr) const
{
    for (size_t mb_index = 0; mb_index < _mb.size(); mb_index++) {
        for (size_t xbar_index = 0;
                xbar_index < _mb[mb_index]->num_xbars;
                xbar_index++) {
            if (_mb.at(mb_index)->get_xbar_local_addr(xbar_index) == xbar_addr) {
                return mb_index;
            }
        }
    }
    throw uhd::lookup_error(str(
        boost::format("Cannot identify mboard for crossbar address %d")
        % xbar_addr
    ));
}

both_xports_t mpmd_impl::make_transport(
        const sid_t& dst_address,
        usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args
) {
    const size_t mb_index =
        identify_mboard_by_xbar_addr(dst_address.get_dst_addr());

    const sid_t sid(
        0, 0, // Not actually an address, more of an 'ignore me' value
        dst_address.get_dst_addr(),
        dst_address.get_dst_endpoint()
    );
    UHD_LOGGER_TRACE("MPMD")
        << "Creating new transport to mboard: " << mb_index
        << " SID: " << sid.to_pp_string_hex()
        << " User-defined xport args: " << args.to_string()
    ;

    both_xports_t xports = _mb[mb_index]->make_transport(
        sid,
        xport_type,
        args
    );
    UHD_LOGGER_TRACE("MPMD")
        << "xport info: send_sid==" << xports.send_sid.to_pp_string_hex()
        << " recv_sid==" << xports.recv_sid.to_pp_string_hex()
        << " endianness=="
            << (xports.endianness == uhd::ENDIANNESS_BIG ? "BE" : "LE")
        << " recv_buff_size==" << xports.recv_buff_size
        << " send_buff_size==" << xports.send_buff_size
    ;
    return xports;
}

