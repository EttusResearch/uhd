//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/adapter_id.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/usrp/common/io_service_args.hpp>
#include <memory>

namespace uhd { namespace usrp {

/*! Class to create and manage I/O services
 *
 * The I/O service manager connects links to I/O services, instantiating new I/O
 * services as needed. It chooses the I/O service to connect based on options
 * from the user passed through stream_args, as well as defaults provided by the
 * caller.
 *
 * The I/O service manager supports two types of I/O services: inline I/O service
 * and offload I/O service. Inline I/O services execute all I/O in the caller
 * thread. Offload I/O services execute all I/O in an offload thread. The offload
 * thread can be configured to block or poll. All control links use inline I/O
 * services, only RX and TX data links currently use offload I/O services.
 *
 * If polling I/O services are requested, the I/O service manager instantiates
 * the number of I/O services specified by the user through args. It chooses
 * which I/O service to connect a set of links to by selecting the I/O service
 * with the fewest number of connections.
 *
 * If blocking I/O services are requested, the I/O service manager instantiates
 * one offload I/O service for each transport adapter used by a streamer. When
 * there are multiple streamers, this manager creates a separate set of I/O
 * services for each streamer.
 *
 * Offload I/O services have a number of restrictions that must be observed:
 * - Offload I/O services currently do not support links that require frame
 *   buffers to be released in order.
 * - Blocking I/O services should only be used for groups of RX or TX data
 *   transport in the same streamer. Since the I/O service blocks on each
 *   channel, if two streamers were to be configured to share the I/O service,
 *   one streamer would block the progress of the other. The I/O service
 *   manager ensures this restriction is observed by grouping I/O services
 *   and links appropriately.
 * - Blocking I/O services do not currently support muxed links, since the I/O
 *   service is specialized to either RX or TX data and the procedure to configure
 *   a data transport requires both RX and TX clients. The I/O service manager
 *   throws an exception if requested to mux a link configured with a blocking
 *   I/O service.
 */
class io_service_mgr
{
public:
    using sptr = std::shared_ptr<io_service_mgr>;

    virtual ~io_service_mgr() = default;

    /*! Connects a pair of links to an I/O service
     *
     * Call this method to connect a pair of links to an I/O service. For muxed
     * links, the I/O service manager keeps track of the number of muxed
     * connections (the number of times this method has been called with the same
     * links).
     *
     * The last two parameters are ignored for control links.
     *
     * \param recv_link The recv link to connect to an I/O service
     * \param send_link The send link to connect to an I/O service
     * \param link_type The type of transport in which the links will be used
     * \param io_srv_args The default stream args for the device
     * \param stream_args The user-provided stream args
     * \param streamer_id A unique ID for the streamer that will use the links
     * \return The I/O service to which the links are connected
     */
    virtual transport::io_service::sptr connect_links(
        transport::recv_link_if::sptr recv_link,
        transport::send_link_if::sptr send_link,
        const transport::link_type_t link_type,
        const io_service_args_t& default_args = io_service_args_t(),
        const uhd::device_addr_t& stream_args = uhd::device_addr_t(),
        const std::string& streamer_id        = "") = 0;

    /*! Disconnects links from their I/O service
     *
     * \param recv_link The recv link to disconnect from an I/O service
     * \param send_link The send link to disconnect from an I/O service
     */
    virtual void disconnect_links(transport::recv_link_if::sptr recv_link,
        transport::send_link_if::sptr send_link) = 0;

    /*! Creates an instance of an I/O service manager
     *
     * \params Device args used to create the UHD session
     * \return The I/O service manager instance
     */
    static sptr make(const uhd::device_addr_t& args);
};

}} // namespace uhd::usrp
