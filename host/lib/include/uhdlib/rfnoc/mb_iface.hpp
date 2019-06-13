//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MB_IFACE_HPP
#define INCLUDED_LIBUHD_MB_IFACE_HPP

#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

// FIXME: Update this
class chdr_rx_data_xport
{
public:
    using uptr = std::unique_ptr<chdr_rx_data_xport>;
};

using chdr_tx_data_xport = chdr_rx_data_xport;

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

    /*! Return a reference to a clock iface
     */
    virtual std::shared_ptr<clock_iface> get_clock_iface(const std::string& clock_name) = 0;

    /*! Create a control transport
     *
     * This is usually called once per motherboard, since there is only one
     * control transport required to talk to all the blocks on the control
     * crossbar.
     *
     * \param local_device_id ID for the host transport adapter to use
     * \param local_epid Host streaming endpoint ID
     * \return A CHDR control transport
     */
    virtual chdr_ctrl_xport::sptr make_ctrl_transport(
        device_id_t local_device_id, const sep_id_t& local_epid) = 0;

    /*! Create an RX data transport
     *
     * This is typically called once per streaming channel.
     *
     * \param local_device_id ID for the host transport adapter to use
     * \param local_epid Host (sink) streaming endpoint ID
     * \param remote_epid Remote device (source) streaming endpoint ID
     * \param xport_args Transport configuration args
     * \return A CHDR RX data transport
     */
    virtual chdr_rx_data_xport::uptr make_rx_data_transport(device_id_t local_device_id,
        const sep_id_t& local_epid,
        const sep_id_t& remote_epid,
        const device_addr_t& xport_args) = 0;

    /*! Create an TX data transport
     *
     * This is typically called once per streaming channel.
     *
     * \param local_device_id ID for the host transport adapter to use
     * \param local_epid Host (source) streaming endpoint ID
     * \param remote_epid Remote device (sink) streaming endpoint ID
     * \param xport_args Transport configuration args
     * \return A CHDR TX data transport
     */
    virtual chdr_tx_data_xport::uptr make_tx_data_transport(device_id_t local_device_id,
        const sep_id_t& local_epid,
        const sep_id_t& remote_epid,
        const device_addr_t& xport_args) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_MB_IFACE_HPP */
