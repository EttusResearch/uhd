//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_LINK_STREAM_MANAGER_HPP
#define INCLUDED_LIBUHD_RFNOC_LINK_STREAM_MANAGER_HPP

#include <uhdlib/rfnoc/client_zero.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <uhdlib/rfnoc/epid_allocator.hpp>
#include <uhdlib/rfnoc/mb_iface.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace rfnoc {

/*! A class that is responsible managing all data endpoints, control endpoints and client
 * zero instances accessible via a physical link. There must be one instance this this
 * class per physical link (Ethernet cable, PCIe connection, etc)
 */
class link_stream_manager
{
public:
    using uptr = std::unique_ptr<link_stream_manager>;

    virtual ~link_stream_manager() = 0;

    /*! \brief Get the software device ID associated with this instance
     *
     * \return A vector of addresses for all reachable endpoints
     */
    virtual device_id_t get_self_device_id() const = 0;

    /*! \brief Get all the endpoints reachable from this link
     *
     * \return A vector of addresses for all reachable endpoints
     */
    virtual const std::set<sep_addr_t>& get_reachable_endpoints() const = 0;

    /*! \brief Initialize a control endpoint to the specified destination
     *
     * \param dst_addr The physical address of the destination endpoint
     * \return A pair (source, destination) endpoint IDs for the control stream
     */
    virtual sep_id_pair_t init_ctrl_stream(sep_addr_t dst_addr) = 0;

    /*! \brief Get a register iface (ctrlport endpoint) to a particular block
     *
     * \param dst_epid The endpoint ID of the destination
     * \param block_index The index of the block in the device
     * \param client_clk The clock that is driving the ctrlport slave
     * \param timebase_clk The clock that is driving the timebase
     * \return An interface to the ctrlport endpoint
     */
    virtual ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk) = 0;

    /*! \brief Get a pointer to the client zero instance for the specified EPID
     *
     * \param dst_epid The endpoint ID of the destination
     * \return An interface to the client zero instance
     */
    virtual detail::client_zero::sptr get_client_zero(sep_id_t dst_epid) const = 0;

    /*! \brief Create a data stream
     *
     * \param dst_epid The endpoint ID of the destination
     * \param vc The virtual channel
     * \param xport_args The transport argument
     * \return An transport instance
     */
    virtual chdr_data_xport_t create_data_stream(
        sep_addr_t dst_addr, sep_vc_t vc, const device_addr_t& xport_args) = 0;

    static uptr make(const chdr::chdr_packet_factory& pkt_factory,
        mb_iface& mb_if,
        const epid_allocator::sptr& epid_alloc,
        device_id_t device_id);

}; // class link_stream_manager

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_LINK_STREAM_MANAGER_HPP */
