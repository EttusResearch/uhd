//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_CTRL_BASE_HPP
#define INCLUDED_MPMD_XPORT_CTRL_BASE_HPP

#include "mpmd_xport_mgr.hpp"
#include "../device3/device3_impl.hpp"
#include <uhd/types/device_addr.hpp>
#include <memory>

namespace uhd { namespace mpmd { namespace xport {

/*! Transport manager implementation base
 */
class mpmd_xport_ctrl_base
{
public:
    using uptr = std::unique_ptr<mpmd_xport_ctrl_base>;
    virtual ~mpmd_xport_ctrl_base() {}

    /*! This is the final step of a make_transport() sequence
     *
     * \param xport_info Contains all necessary transport info. The
     *                   implementation may update this!
     * \param xport_type CTRL, ASYNC_MSG, ... (see xport_type_t)
     * \param xport_args Additional arguments. These can come from the user.
     */
    virtual both_xports_t make_transport(
        mpmd_xport_mgr::xport_info_t& xport_info,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args
    ) = 0;

    //! Assert if an xport_info is even valid/feasible/available
    virtual bool is_valid(
        const mpmd_xport_mgr::xport_info_t& xport_info
    ) const = 0;

    virtual size_t get_mtu(
        const uhd::direction_t dir
    ) const = 0;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_CTRL_BASE_HPP */
