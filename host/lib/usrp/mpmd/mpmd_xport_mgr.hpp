//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_MGR_HPP
#define INCLUDED_MPMD_XPORT_MGR_HPP

#include "../device3/device3_impl.hpp"
#include <uhd/types/dict.hpp>
#include <memory>
#include <map>
#include <vector>
#include <string>

namespace uhd { namespace mpmd { namespace xport {

/*
 * Transport specifiers
 */

//! Ethernet address for management and RPC communication
const std::string MGMT_ADDR_KEY    = "mgmt_addr";
//! Primary Ethernet address for streaming and RFNoC communication
const std::string FIRST_ADDR_KEY   = "addr";
//! Secondary Ethernet address for streaming and RFNoC communication
const std::string SECOND_ADDR_KEY  = "second_addr";

/*! Return filtered subset from a device_addr_t
 *
 * The return dictionary will contain all key/value pairs from \p args
 * where the key begins with \p prefix.
 *
 * \param args Bucket of key/value pairs
 * \param prefix Key prefix to match against
 */
uhd::dict<std::string, std::string> filter_args(
    const uhd::device_addr_t& args,
    const std::string& prefix
);

/*! MPMD Transport Manager
 *
 * A transport manager is an object which sets up a physical connection to a
 * CHDR device. Its implementation is specific to the underlying transport
 * medium. For example, if the medium is Ethernet/UDP, this class will create
 * sockets.
 */
class mpmd_xport_mgr
{
public:
    using uptr = std::unique_ptr<mpmd_xport_mgr>;
    using xport_info_t = std::map<std::string, std::string>;
    using xport_info_list_t = std::vector<std::map<std::string, std::string>>;
    virtual ~mpmd_xport_mgr() {}

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
    static uptr make(
        const uhd::device_addr_t& mb_args
    );

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
    virtual both_xports_t make_transport(
        const xport_info_list_t &xport_info_list,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args,
        xport_info_t& xport_info_out
    ) = 0;

    /*! Return the path MTU for whatever this manager lets us do
     */
    virtual size_t get_mtu(
        const uhd::direction_t dir
    ) const = 0;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_MGR_HPP */
