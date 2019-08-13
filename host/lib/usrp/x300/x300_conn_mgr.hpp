//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_CONN_MGR_HPP
#define INCLUDED_X300_CONN_MGR_HPP

#include <uhd/transport/if_addrs.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
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
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_CONN_MGR_HPP */
