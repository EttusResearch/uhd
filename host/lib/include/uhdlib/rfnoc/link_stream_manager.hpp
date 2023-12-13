//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/adapter_id.hpp>
#include <uhdlib/rfnoc/chdr_rx_data_xport.hpp>
#include <uhdlib/rfnoc/client_zero.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <uhdlib/rfnoc/epid_allocator.hpp>
#include <uhdlib/rfnoc/mb_iface.hpp>
#include <uhdlib/rfnoc/topo_graph.hpp>
#include <functional>
#include <memory>
#include <set>

namespace uhd { namespace rfnoc {

/*! A class that is responsible for managing all data endpoints, control endpoints and
 * client zero instances accessible via a logical link between the host device and
 * motherboard.
 *
 * Note that each transport adapter on the host has its own set of streaming endpoints,
 * and thus, the host's device_id_t uniquely identifies the host-side transport adapter
 * to use for packet transmission/reception.
 *
 * For convenience, the link_stream_manager also provides a method to get the
 * host's transport adapter ID directly.
 *
 * There must be one instance of this class per logical link. This means there
 * is at least one link_stream_manager per USRP attached to an rfnoc_graph, and
 * if the user requested multiple links (e.g., using `addr=...,second_addr=')
 * then there are multiple link_stream_managers.
 */
class link_stream_manager
{
public:
    using uptr = std::unique_ptr<link_stream_manager>;

    virtual ~link_stream_manager() = 0;

    /*! Find transport adapters unreachable by discovery and add them to graph
     *
     * Call this after all link stream managers are initialized. It will query
     * the device for a list of all transport adapters, and add those to the
     * graph which were not previously found by the topology discovery. This
     * implies that transport adapters found with this method are never
     * available for communication with UHD directly, and can only be used to
     * stream elsewhere (e.g., using the raw UDP streaming API).
     */
    virtual void add_unreachable_transport_adapters() = 0;

    /*! \brief Get the software device ID associated with this instance
     *
     * For every link to a device, we create a unique device ID. For example,
     * if there are two USRPs in the graph, each connected with a single
     * Ethernet connection, there would be two link managers, and therefore also
     * two device IDs on the host side.
     * If we access a single USRP using two Ethernet connections, then we still
     * have two link stream managers, each with its own unique device ID on the
     * host side.
     * The device IDs are allocated in the mb_iface associated with this device
     * during discovery.
     *
     * \return The software device ID associated with this instance
     */
    virtual device_id_t get_self_device_id() const = 0;

    /*! \brief Get the transport adapter ID associated with this instance
     *
     * See also uhd::transport::adapter_id_t. For example, when using two
     * separate Ethernet ports, there would be two adapter IDs.
     *
     * \return The adapter ID associated with this instance
     */
    virtual uhd::transport::adapter_id_t get_adapter_id() const = 0;

    /*! \brief Get all the endpoints reachable from this link
     *
     * \return A vector of addresses for all reachable endpoints
     */
    virtual std::set<sep_addr_t> get_reachable_endpoints() const = 0;

    /*! \brief Connect the host to the specified destination and init a control endpoint
     *
     * \param dst_addr The physical address of the destination endpoint
     * \return A pair (source, destination) endpoint IDs for the control stream
     */
    virtual sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr) = 0;

    /*! \brief Check if the two specified endpoints can be connected remotely
     * by this link stream manager instance.
     *
     * Note: If this returns, a connection may still be possible, but might
     * require a different link stream manager instance.
     *
     * \param dst_addr The physical address of the destination endpoint
     * \param src_addr The physical address of the source endpoint
     * \return true if the endpoints can be connected by this link stream manager
     *              instance.
     */
    virtual bool can_connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) const = 0;

    /*! \brief Connect two remote endpoints to each other
     *
     * \param dst_addr The physical address of the destination endpoint
     * \param src_addr The physical address of the source endpoint
     * \return A pair (source, destination) endpoint IDs for the src/dst
     */
    virtual sep_id_pair_t connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) = 0;

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

    /*! Configure a flow controlled data stream from the endpoint with ID src_epid to the
     *  endpoint with ID dst_epid
     *
     * \param dst_epid The endpoint ID of the destination
     * \param src_epid The endpoint ID of the source
     * \param lossy_xport Is the transport lossy?
     * \param fc_freq_ratio Flow control response frequency as a ratio of the buff params
     * \param fc_headroom_ratio Flow control headroom as a ratio of the buff params
     * \param reset Optionally reset the stream
     */
    virtual stream_buff_params_t create_device_to_device_data_stream(
        const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const bool reset = false) = 0;

    /*! \brief Create a data stream going from the host to the device
     *
     * \param dst_addr The address of the destination stream endpoint
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param xport_args The transport arguments
     * \param streamer_id A unique identifier for the streamer that will own the transport
     * \return An transport instance
     */
    virtual chdr_tx_data_xport::uptr create_host_to_device_data_stream(
        const sep_addr_t dst_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) = 0;

    /*! \brief Create a data stream going from the device to the host
     *
     * \param dst_addr The address of the destination stream endpoint
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param xport_args The transport arguments
     * \param streamer_id A unique identifier for the streamer that will own the transport
     * \return An transport instance
     */
    virtual chdr_rx_data_xport::uptr create_device_to_host_data_stream(
        const sep_addr_t src_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) = 0;

    static uptr make(const chdr::chdr_packet_factory& pkt_factory,
        mb_iface& mb_if,
        const epid_allocator::sptr& epid_alloc,
        device_id_t device_id,
        detail::topo_graph_t::sptr topo_graph);

}; // class link_stream_manager

}} /* namespace uhd::rfnoc */
