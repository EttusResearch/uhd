//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MB_IFACE_HPP
#define INCLUDED_LIBUHD_MB_IFACE_HPP

#include <memory>

namespace uhd { namespace rfnoc {


/*! Motherboard (backchannel) interface
 *
 * In RFNoC devices, the RFNoC subystem needs a backchannel interface to talk to
 * the individual motherboards. Every rfnoc_graph needs one interface per
 * attached motherboard.
 *
 * It's up to the various device implementations (e.g., x300_impl) to implement
 * this interface.
 */
class mb_iface {
public:
    using uptr = std::unique_ptr<mb_iface>;

    virtual ~mb_iface() = 0;

    /*! Return the RFNoC protocol version for this motherboard
     */
    virtual uint16_t get_proto_ver() = 0;

    /*! Return the CHDR width for this motherboard
     */
    virtual chdr_w_t get_chdr_width() = 0;

    /*! Set the device ID of this motherboard
     *
     * Every motherboard in a multi-USRP setup needs a unique device ID. It is
     * up to the rfnoc_graph to choose the various IDs, and then call this
     * function to set it.
     */
    virtual void set_device_id(const uint16_t id) = 0;

    /*! Get device ID
     *
     * Returns the value previously written by set_device_id(). A freshly
     * resetted motherboard which has not been assigned a device ID should
     * return 0xFFFF.
     *
     * \returns the motherboard's device ID
     */
    virtual uint16_t get_device_id() = 0;

    /*! Reset the device
     */
    virtual void reset_network() = 0;

    /*! Return a list of all physical links available from the current UHD
     * session to the motherboard.
     *
     * FIXME determine appropriate return type
     */
    //virtual <link info> enumerate_links() = 0;

    /*!
     *
     * FIXME determine appropriate return type and arg types
     */
    //virtual link_iface::uptr create_link(<link_params>) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_MB_IFACE_HPP */
