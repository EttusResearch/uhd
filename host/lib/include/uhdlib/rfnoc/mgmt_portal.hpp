//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MGMT_PORTAL_HPP
#define INCLUDED_LIBUHD_MGMT_PORTAL_HPP

#include <uhdlib/rfnoc/chdr_types.hpp>
#include <memory>

namespace uhd { namespace rfnoc { namespace mgmt {

//! A portal to perform low-level management operations from an endpoint
//
// This object provides an interface to send management commands from a software stream
// endpoint. There must one instance of this object per software stream endpoint.
// The management portal is capable of discovering all endpoints reachable from the
// transport associated with it. It can then setup routes and configure stream endpoints
// downstream.
class mgmt_portal
{
public:
    using uptr           = std::unique_ptr<mgmt_portal>;
    using xport_cfg_fn_t = std::function<void(
        device_id_t devid, uint16_t inst, uint8_t subtype, chdr::mgmt_hop_t& hop)>;

    //! Flow control buffer configuration parameters
    struct buff_params_t
    {
        uint64_t bytes;
        uint32_t packets;
    };

    //! Information about a stream endpoint
    struct sep_info_t
    {
        //! Does the endpoint support control traffic?
        bool has_ctrl;
        //! Does the endpoint support data traffic?
        bool has_data;
        //! Number of input data ports
        size_t num_input_ports;
        //! Number of output data ports
        size_t num_output_ports;
        //! Does the endpoint send a stream status packet in case of errors
        bool reports_strm_errs;
        //! Address of the endpoint
        sep_addr_t addr;
    };

    //! The data type of the buffer used to capture/generate data
    enum sw_buff_t { BUFF_U64 = 0, BUFF_U32 = 1, BUFF_U16 = 2, BUFF_U8 = 3 };

    virtual ~mgmt_portal() = 0;

    //! Get addresses for all stream endpoints reachable from this SW mgmt portal
    //  Note that the endpoints that are not physically connected/reachable from
    //  the underlying transport will not be discovered.
    //
    virtual const std::set<sep_addr_t>& get_reachable_endpoints() const = 0;

    //! Initialize a stream endpoint and assign an endpoint ID to it
    //
    // \param addr The physical address of the stream endpoint
    // \param epid The endpoint ID to assign to this endpoint
    //
    virtual void initialize_endpoint(const sep_addr_t& addr, const sep_id_t& epid) = 0;

    //! Get information about a discovered (reachable) stream endpoint
    //
    // \param epid The endpoint ID of the endpoint to lookup
    //
    virtual bool is_endpoint_initialized(const sep_id_t& epid) const = 0;

    //! Get information about a discovered (reachable) stream endpoint
    //
    // \param epid The endpoint ID of the endpoint to lookup
    //
    virtual sep_info_t get_endpoint_info(const sep_id_t& epid) const = 0;

    //! Setup a route from this SW mgmt portal to the specified destination endpoint
    //
    //  After a route is established, it should be possible to send packets to the
    //  destination simply by setting the DstEPID in the CHDR header to the specified
    //  dst_epid
    //
    // \param dst_epid The endpoint ID of the destination
    //
    virtual void setup_local_route(const sep_id_t& dst_epid) = 0;

    //! Setup a route from between the source and destination endpoints
    //
    //  After a route is established, it should be possible for the source to send packets
    //  to the destination simply by setting the DstEPID in the CHDR header to the
    //  specified dst_epid
    //
    // \param dst_epid The endpoint ID of the destination
    // \param src_epid The endpoint ID of the source
    //
    virtual void setup_remote_route(
        const sep_id_t& dst_epid, const sep_id_t& src_epid) = 0;

    //! Start configuring a flow controlled receive data stream from the endpoint with the
    //  specified ID to this SW mgmt portal.
    //
    //  RX stream setup is a two-step process. After this function is called, the flow
    //  control handler needs to acknoweledge the setup transaction then call the commit
    //  function below.
    //
    // \param epid The endpoint ID of the data source
    // \param lossy_xport Is the transport lossy? (e.g. UDP, not liberio)
    // \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
    // \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
    // \param fc_freq Flow control response frequency parameters
    // \param fc_freq Flow control headroom parameters
    // \param reset Reset ingress stream endpoint state
    //
    virtual void config_local_rx_stream_start(const sep_id_t& epid,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const buff_params_t& fc_freq,
        const buff_params_t& fc_headroom,
        const bool reset = false) = 0;

    //! Finish configuring a flow controlled receive data stream from the endpoint with
    //  the specified ID to this SW mgmt portal.
    //
    // \param epid The endpoint ID of the data source
    //
    virtual buff_params_t config_local_rx_stream_commit(
        const sep_id_t& epid, const double timeout = 0.2) = 0;

    //! Configure a flow controlled transmit data stream from this SW mgmt portal to the
    //  endpoint with the specified ID.
    //
    // \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
    // \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
    // \param reset Reset ingress stream endpoint state
    //
    virtual void config_local_tx_stream(const sep_id_t& epid,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const bool reset = false) = 0;

    //! Configure a flow controlled data stream from the endpoint with ID src_epid to the
    //  endpoint with ID dst_epid
    //
    // \param dst_epid The endpoint ID of the destination
    // \param src_epid The endpoint ID of the source
    // \param lossy_xport Is the transport lossy?
    // \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
    // \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
    // \param fc_freq Flow control response frequency parameters
    // \param fc_freq Flow control headroom parameters
    //
    virtual buff_params_t config_remote_stream(const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const buff_params_t& fc_freq,
        const buff_params_t& fc_headroom,
        const bool reset     = false,
        const double timeout = 0.2) = 0;

    //! Define custom configuration functions for custom transports
    //
    // \param xport_type The type of the custom transport
    // \param init_hop_cfg_fn The function to call when initializing the custom xport
    // \param rtcfg_hop_cfg_fn The function to call when configuring routing for the
    // custom xport
    //
    virtual void register_xport_hop_cfg_fns(uint8_t xport_subtype,
        xport_cfg_fn_t init_hop_cfg_fn,
        xport_cfg_fn_t rtcfg_hop_cfg_fn) = 0;

    //! Create an endpoint manager object
    //
    static uptr make(const chdr_ctrl_xport_t& xport,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_addr_t my_sep_addr,
        sep_id_t my_epid);
};

}}} // namespace uhd::rfnoc::mgmt

#endif /* INCLUDED_LIBUHD_MGMT_PORTAL_HPP */
