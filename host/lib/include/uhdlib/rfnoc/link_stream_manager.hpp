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
#include <set>

namespace uhd { namespace rfnoc {

/*! A class that is responsible managing all data endpoints, control endpoints and client
 * zero instances accessible via a logical link between the host device and
 * motherboard.
 *
 * Note that each transport adapter on the host has its own set of streaming endpoints,
 * and thus, the host's device_id_t uniquely identifies the host-side transport adapter
 * to use for packet transmission/reception.
 *
 * There must be one instance of this class per logical link.
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

    /*! \brief Connect the host to the specified destination and init a control endpoint
     *
     * \param dst_addr The physical address of the destination endpoint
     * \return A pair (source, destination) endpoint IDs for the control stream
     */
    virtual sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr) = 0;

    /*! \brief Check if the two specified endpoints can be connected remotely
     *
     * \param dst_addr The physical address of the destination endpoint
     * \param src_addr The physical address of the source endpoint
     * \return true if the endpoints can be connected
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
     * \param lossy_xport Is the transport lossy?
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param fc_freq_ratio Flow control response frequency as a ratio of the buff params
     * \param fc_headroom_ratio Flow control headroom as a ratio of the buff params
     * \param xport_args The transport arguments
     * \return An transport instance
     */
    virtual chdr_data_xport_t create_host_to_device_data_stream(const sep_addr_t dst_addr,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const device_addr_t& xport_args) = 0;

    /*! \brief Create a data stream going from the device to the host
     *
     * \param dst_addr The address of the destination stream endpoint
     * \param lossy_xport Is the transport lossy?
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param fc_freq_ratio Flow control response frequency as a ratio of the buff params
     * \param fc_headroom_ratio Flow control headroom as a ratio of the buff params
     * \param xport_args The transport arguments
     * \return An transport instance
     */
    virtual chdr_data_xport_t create_device_to_host_data_stream(const sep_addr_t src_addr,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const device_addr_t& xport_args) = 0;

    static uptr make(const chdr::chdr_packet_factory& pkt_factory,
        mb_iface& mb_if,
        const epid_allocator::sptr& epid_alloc,
        device_id_t device_id);

}; // class link_stream_manager

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_LINK_STREAM_MANAGER_HPP */
