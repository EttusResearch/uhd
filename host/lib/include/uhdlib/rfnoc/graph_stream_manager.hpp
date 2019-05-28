//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_GRAPH_STREAM_MANAGER_HPP
#define INCLUDED_LIBUHD_RFNOC_GRAPH_STREAM_MANAGER_HPP

#include <uhdlib/rfnoc/chdr_packet.hpp>
#include <uhdlib/rfnoc/client_zero.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <uhdlib/rfnoc/epid_allocator.hpp>
#include <uhdlib/rfnoc/mb_iface.hpp>
#include <functional>
#include <memory>
#include <set>

namespace uhd { namespace rfnoc {

/*! A class that is responsible managing all data endpoints, control endpoints and client
 * zero instances accessible through this graph. There must be one instance of this
 * class per graph
 */
class graph_stream_manager
{
public:
    using uptr = std::unique_ptr<graph_stream_manager>;

    virtual ~graph_stream_manager() = 0;

    /*! \brief Get all the local devices that can be taken from this graph
     *
     * \return A vector of IDs of all local devices
     */
    virtual std::vector<device_id_t> get_local_devices() const = 0;

    /*! \brief Get all the endpoints reachable from this graph
     *
     * \return A set of addresses for all reachable endpoints
     */
    virtual const std::set<sep_addr_t>& get_reachable_endpoints() const = 0;

    /*! \brief Initialize a control endpoint to the specified destination
     *
     * \param dst_addr The physical address of the destination endpoint
     * \param via_device The preference for the device to take to get to the destination
     * \return A pair (source, destination) endpoint IDs for the control stream
     */
    virtual sep_id_pair_t init_ctrl_stream(
        sep_addr_t dst_addr, device_id_t via_device = NULL_DEVICE_ID) = 0;

    /*! \brief Get a register iface (ctrlport endpoint) to a particular block
     *
     * \param dst_epid The endpoint ID of the destination
     * \param block_index The index of the block in the device
     * \param client_clk The clock that is driving the ctrlport slave
     * \param timebase_clk The clock that is driving the timebase
     * \param via_device The preference for the device to take to get to the destination
     * \return An interface to the ctrlport endpoint
     */
    virtual ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk,
        device_id_t via_device = NULL_DEVICE_ID) = 0;

    /*! \brief Get a pointer to the client zero instance for the specified EPID
     *
     * \param dst_epid The endpoint ID of the destination
     * \param via_device The preference for the device to take to get to the destination
     * \return An interface to the client zero instance
     */
    virtual detail::client_zero::sptr get_client_zero(
        sep_id_t dst_epid, device_id_t via_device = NULL_DEVICE_ID) const = 0;

    static uptr make(const chdr::chdr_packet_factory& pkt_factory,
        const epid_allocator::sptr& epid_alloc,
        const std::vector<std::pair<device_id_t, mb_iface*>>& links);

}; // class graph_stream_manager

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_GRAPH_STREAM_MANAGER_HPP */
