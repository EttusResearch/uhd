//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MB_IFACE_HPP
#define INCLUDED_LIBUHD_MB_IFACE_HPP

#include <uhdlib/rfnoc/rfnoc_common.hpp>
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
class mb_iface
{
public:
    using uptr = std::unique_ptr<mb_iface>;

    virtual ~mb_iface() = default;

    /*! Return the RFNoC protocol version of the firmware running on this motherboard
     */
    virtual uint16_t get_proto_ver() = 0;

    /*! Return the CHDR width of the firmware running on this motherboard
     */
    virtual chdr_w_t get_chdr_w() = 0;

    /*! Get the device ID assigned to the motherboard
     *
     * A freshly reset motherboard should return 0.
     *
     * \returns the motherboard's device ID
     */
    virtual device_id_t get_remote_device_id() = 0;

    /*! Get the local (software) device IDs on this motherboard that can actively
     * communicate with the sea of RFNoC FPGAs. The number of local devices returned
     * should be equal to the number of physical links on the motherboard that are
     * actively connected.
     *
     * \returns The active software device IDs
     */
    virtual std::vector<device_id_t> get_local_device_ids() = 0;

    /*! Reset the device
     */
    virtual void reset_network() = 0;

    /*! Create a control transport
     */
    virtual chdr_ctrl_xport_t make_ctrl_transport(
        device_id_t local_device_id, const sep_id_t& src_epid) = 0;

    /*! Create a data transport
     */
    virtual chdr_data_xport_t make_data_transport(device_id_t local_device_id,
        const sep_id_t& src_epid,
        const device_addr_t& xport_args) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_MB_IFACE_HPP */
