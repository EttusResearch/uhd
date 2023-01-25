//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/transport/adapter_id.hpp>
#include <uhd/types/endianness.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>
#include <uhdlib/rfnoc/chdr_rx_data_xport.hpp>
#include <uhdlib/rfnoc/chdr_tx_data_xport.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/usrp/common/io_service_mgr.hpp>
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

    /*! Return the endianness for the link associated with \p local_device_id
     *
     * When \p local_device_id is set to NULL_DEVICE_ID, it will return any
     * endianness associated with this device.
     */
    virtual uhd::endianness_t get_endianness(
        const device_id_t local_device_id = NULL_DEVICE_ID) = 0;

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

    /*! Return the uhd::transport::adapter_id for the link associated with
     * \p local_device_id
     *
     * A transport must be created on the adapter before this will be valid.
     * Since link_stream_managers always create a chdr_ctrl_xport on
     * construction, the requirement is satisfied.
     */
    virtual uhd::transport::adapter_id_t get_adapter_id(
        const device_id_t local_device_id) = 0;

    /*! Reset the device
     */
    virtual void reset_network() = 0;

    /*! Return a reference to a clock iface
     */
    virtual std::shared_ptr<clock_iface> get_clock_iface(
        const std::string& clock_name) = 0;

    /*! Set the IO service manager
     *
     */
    void set_io_srv_mgr(uhd::usrp::io_service_mgr::sptr io_srv_mgr)
    {
        _io_srv_mgr = io_srv_mgr;
    }

    /*! Get the I/O Service Manager
     *
     * This function must be called by the implementations of the various
     * make_*_transport() calls to get access to the global I/O Service Manager
     */
    uhd::usrp::io_service_mgr::sptr get_io_srv_mgr()
    {
        if (!_io_srv_mgr) {
            throw uhd::runtime_error("I/O Service Manager not set for mb_iface!");
        }
        return _io_srv_mgr;
    }

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
     * \param addrs Address of the device and host stream endpoints
     * \param epids Endpoint IDs of the device and host stream endpoints
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param xport_args Transport configuration args
     * \param streamer_id A unique identifier for the streamer that will own the transport
     * \return A CHDR RX data transport
     */
    virtual chdr_rx_data_xport::uptr make_rx_data_transport(
        mgmt::mgmt_portal& mgmt_portal,
        const sep_addr_pair_t& addrs,
        const sep_id_pair_t& epids,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) = 0;

    /*! Create an TX data transport
     *
     * This is typically called once per streaming channel.
     *
     * \param addrs Address of the host and device stream endpoints
     * \param epids Endpoint IDs of the host and device stream endpoints
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param xport_args Transport configuration args
     * \param streamer_id A unique identifier for the streamer that will own the transport
     * \return A CHDR TX data transport
     */
    virtual chdr_tx_data_xport::uptr make_tx_data_transport(
        mgmt::mgmt_portal& mgmt_portal,
        const sep_addr_pair_t& addrs,
        const sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) = 0;

    /*! List transport adapters that can be used for remote CHDR streaming and
     * information about them.
     *
     * \returns A map of type name -> adapter info. The keys of this map are the
     *          available transport adapters by name (e.g., "sfp0").  These can
     *          be used in add_remote_chdr_route().
     *          The values are dictionaries that provide additional information
     *          about the transport adapter.
     */
    virtual std::map<std::string, device_addr_t> get_chdr_xport_adapters() = 0;

    /*! Create a route from a transport adapter to a remote endpoint
     *
     * This can be called when creating a CHDR route from a device to a remote
     * destination.
     *
     * The main information is carried in the \p route_args dictionary. How
     * exactly the dictionary should be populated depends on the type of
     * transport and the device. When using UDP streams, this dictionary requires
     * at least the following keys:
     * - addr (destination IP address)
     * - port (destination UDP port)
     * - stream_mode (how to format outgoing packets)
     *
     * \param adapter_id A string identifier for an adapter (e.g., 'sfp0').
     * \param epid The endpoint ID for which this route is being set up
     * \param route_args Routing arguments (see above)
     *
     * \returns An integer value representing the transport adapter used for
     *          this route.
     */
    virtual int add_remote_chdr_route(const std::string& adapter_id,
        const sep_id_t epid,
        const device_addr_t& route_args) = 0;
    // TODO: This implies that all routes are outgoing, what if we want to add
    // a route for incoming traffic that should get CHDR'd up (i.e., doesn't
    // carry a CHDR header of its own)?


private:
    uhd::usrp::io_service_mgr::sptr _io_srv_mgr;
};

}} /* namespace uhd::rfnoc */
