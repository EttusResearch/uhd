//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_LINK_IF_MGR_HPP
#define INCLUDED_MPMD_LINK_IF_MGR_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/direction.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/links.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace mpmd { namespace xport {

/*
 * Transport specifiers
 */

//! Primary Ethernet address for streaming and RFNoC communication
const std::string FIRST_ADDR_KEY = "addr";
//! Secondary Ethernet address for streaming and RFNoC communication
const std::string SECOND_ADDR_KEY = "second_addr";
//! Tertiary Ethernet address for streaming and RFNoC communication
const std::string THIRD_ADDR_KEY = "third_addr";
//! Quaternary Ethernet address for streaming and RFNoC communication
const std::string FOURTH_ADDR_KEY = "fourth_addr";

/*! Return filtered subset from a device_addr_t
 *
 * The return dictionary will contain all key/value pairs from \p args
 * where the key begins with \p prefix.
 *
 * \param args Bucket of key/value pairs
 * \param prefix Key prefix to match against
 */
uhd::dict<std::string, std::string> filter_args(
    const uhd::device_addr_t& args, const std::string& prefix);

/*! MPMD Transport Manager
 *
 * A transport manager is a factory object which sets up a physical connection to a
 * CHDR device. Its implementation is specific to the underlying transport
 * medium. For example, if the medium is Ethernet/UDP, this class will create
 * sockets.
 *
 * Note: As of UHD 4.0, there is only one underlying transport medium (UDP).
 * We keep this factory nevertheless for the sake of continuity and future-proofing.
 */
class mpmd_link_if_mgr
{
public:
    using uptr              = std::unique_ptr<mpmd_link_if_mgr>;
    using xport_info_t      = std::map<std::string, std::string>;
    using xport_info_list_t = std::vector<std::map<std::string, std::string>>;
    virtual ~mpmd_link_if_mgr() {}

    /*! Return a reference to a transport manager
     *
     * \param mb_args Additional args from the motherboard. These may contain
     *                transport-related args (e.g., "recv_buff_size") which
     *                can be relevant to the underlying implementation.
     *
     * \returns Reference to manager object
     * \throws uhd::key_error if \p xport_medium is not supported. The ctor of
     *         the underlying class that is requested can also throw.
     */
    static uptr make(const uhd::device_addr_t& mb_args);

    /*! Attempt to open a CHDR-capable link to the remote device
     *
     * This will compare the mb_args (passed in at construction) with
     * \p xport_info to see if it can connect this way. For example, if
     * \p xport_type is "udp", then it will see if it can find the `addr` key
     * from mb_args in the \p xport_info. If yes, it will use that for
     * connections.
     *
     * \param xport_type The type of xport ("udp")
     * \param xport_info The available information on this transport. For
     *                   example, if the xport_type is "udp", then this would
     *                   contain the available IP addresses.
     * \returns true on success
     */
    virtual bool connect(const std::string& xport_type,
        const xport_info_list_t& xport_info,
        const uhd::rfnoc::chdr_w_t chdr_w) = 0;

    /*! The number of available links
     *
     * If zero, it means that there is no valid connection to the device.
     *
     */
    virtual size_t get_num_links() = 0;

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

    /*! Create a transports object
     *
     * Implementation details depend on the underlying implementation.
     * In general, the implementations will follow the following recipe:
     * 1. Pick a suitable element from \p xport_info_list
     * 2. Do whatever system calls are necessary to create the physical
     *    transport; to do so, call the underlying implementation (UDP or
     *    whatever)
     * 3. Update the selected element from xport_info_list
     * 5. Return results
     *
     * \param xport_info_list List of possible options to choose from. Every
     *                        element of this argument needs to have the same
     *                        "type" key (e.g., they all need to be "UDP").
     * \param xport_type Transport type (CTRL, RX_DATA, ...)
     * \param xport_args Arbitrary additional transport args. These could come
     *                   from the user, or other places.
     * \param xport_info_out The updated dictionary from xport_info_list that
     *                       was eventually chosen
     *
     * \returns The both_xports_t object containing the actual transport info,
     *          and xport_info_out contains the updated transport option info.
     *          The latter needs to get sent back to MPM to complete the
     *          transport handshake.
     */
    // virtual both_xports_t make_transport(const xport_info_list_t& xport_info_list,
    // const uhd::transport::link_type_t::xport_type_t xport_type,
    // const uhd::device_addr_t& xport_args,
    // xport_info_t& xport_info_out) = 0;

    /*! Return the path MTU for whatever this manager lets us do
     */
    virtual size_t get_mtu(const size_t link_idx, const uhd::direction_t dir) const = 0;

    /*! Get packet factory from associated link_mgr
     *
     * \param link_idx The number of the link to use. link_idx < get_num_links()
     *                 must hold true. link_idx is often 0. Example: When
     *                 the underlying transport is Ethernet, and the user
     *                 specified both addr and second_addr, then get_num_links()
     *                 equals 2 and link_idx can also be 1.
     * \return a CHDR packet factory
     */
    virtual const uhd::rfnoc::chdr::chdr_packet_factory& get_packet_factory(
        const size_t link_idx) const = 0;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_LINK_IF_MGR_HPP */
