//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_CONN_MGR_HPP
#define INCLUDED_X300_CONN_MGR_HPP

#include <uhd/transport/if_addrs.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/links.hpp>
#include <string>

namespace uhd { namespace usrp { namespace x300 {

/*! Helper base class to manage the connection to the device
 */
class conn_manager
{
public:
    using sptr = std::shared_ptr<conn_manager>;
    virtual ~conn_manager() {}

    /*! Return a reference to a ZPU ctrl interface object
     */
    virtual uhd::wb_iface::sptr get_ctrl_iface() = 0;

    virtual size_t get_mtu(uhd::direction_t dir) = 0;

    virtual std::vector<uhd::rfnoc::device_id_t> get_local_device_ids() = 0;

    virtual uhd::transport::both_links_t get_links(uhd::transport::link_type_t link_type,
        const uhd::rfnoc::device_id_t local_device_id,
        const uhd::rfnoc::sep_id_t& local_epid,
        const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::device_addr_t& link_args) = 0;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_CONN_MGR_HPP */
