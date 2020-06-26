//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_CTRL_BASE_HPP
#define INCLUDED_MPMD_XPORT_CTRL_BASE_HPP

#include "mpmd_link_if_mgr.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/transport/links.hpp>
#include <memory>

namespace uhd { namespace mpmd { namespace xport {

/*! Transport manager implementation base
 */
class mpmd_link_if_ctrl_base
{
public:
    using uptr = std::unique_ptr<mpmd_link_if_ctrl_base>;
    virtual ~mpmd_link_if_ctrl_base() {}

    virtual size_t get_num_links() const = 0;

    /*! Return links object
     *
     * \param link_idx The number of the link to use. link_idx < get_num_links()
     *                 must hold true. link_idx is often 0. Example: When
     *                 the underlying transport is Ethernet, and the user
     *                 specified both addr and second_addr, then get_num_links()
     *                 equals 2 and link_idx can also be 1.
     * \param link_type CTRL, RX_DATA, or TX_DATA (for configuring the link)
     * \param link_args Link-specific additional information that the underlying
     *                  mpmd_link_if_ctrl instantiation can use
     */
    virtual uhd::transport::both_links_t get_link(const size_t link_idx,
        const uhd::transport::link_type_t link_type,
        const uhd::device_addr_t& link_args) = 0;

    //! Return the underlying link's MTU in bytes
    virtual size_t get_mtu(const uhd::direction_t dir) const = 0;

    //! Return the rate of the underlying link in bytes/sec
    virtual double get_link_rate(const size_t link_idx) const = 0;

    //! Get the packet factory associated with this link
    virtual const uhd::rfnoc::chdr::chdr_packet_factory& get_packet_factory() const = 0;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_CTRL_BASE_HPP */
