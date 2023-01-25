//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_rx_data_xport.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::detail;
using namespace uhd::transport;

rx_flow_ctrl_sender::rx_flow_ctrl_sender(
    const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids)
    : _dst_epid(sep_ids.first)
{
    _fc_packet             = pkt_factory.make_strs();
    _fc_strs_pyld.src_epid = sep_ids.second;
}

void rx_flow_ctrl_sender::set_capacity(const stream_buff_params_t& recv_capacity)
{
    _fc_strs_pyld.capacity_bytes = recv_capacity.bytes;
    _fc_strs_pyld.capacity_pkts  = recv_capacity.packets;
}

chdr_rx_data_xport::chdr_rx_data_xport(uhd::transport::io_service::sptr io_srv,
    uhd::transport::recv_link_if::sptr recv_link,
    uhd::transport::send_link_if::sptr send_link,
    const chdr::chdr_packet_factory& pkt_factory,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const size_t num_recv_frames,
    const fc_params_t& fc_params,
    disconnect_callback_t disconnect)
    : _fc_state(epids, fc_params.freq)
    , _mtu(recv_link->get_recv_frame_size())
    , _fc_sender(pkt_factory, epids)
    , _epid(epids.second)
    , _chdr_w_bytes(chdr_w_to_bits(pkt_factory.get_chdr_w()) / 8)
    , _disconnect(disconnect)
{
    UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
        "Creating rx xport with local epid=" << epids.second
                                             << ", remote epid=" << epids.first);

    _recv_packet    = pkt_factory.make_generic();
    _recv_packet_cb = pkt_factory.make_generic();
    _fc_sender.set_capacity(fc_params.buff_capacity);

    // Calculate header size
    _hdr_len = _recv_packet->calculate_payload_offset(chdr::PKT_TYPE_DATA_WITH_TS);
    UHD_ASSERT_THROW(_hdr_len);

    // Make data transport
    auto recv_cb =
        [this](buff_t::uptr& buff, recv_link_if* recv_link, send_link_if* send_link) {
            return this->_recv_callback(buff, recv_link, send_link);
        };

    auto fc_cb =
        [this](buff_t::uptr buff, recv_link_if* recv_link, send_link_if* send_link) {
            this->_fc_callback(std::move(buff), recv_link, send_link);
        };

    // Needs just a single send frame for responses
    _recv_io = io_srv->make_recv_client(recv_link,
        num_recv_frames,
        recv_cb,
        send_link,
        /* num_send_frames*/ 1,
        fc_cb);

    UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
        "Stream endpoint was configured with:"
            << std::endl
            << "capacity bytes=" << fc_params.buff_capacity.bytes
            << ", packets=" << fc_params.buff_capacity.packets << std::endl
            << "fc frequency bytes=" << fc_params.freq.bytes
            << ", packets=" << fc_params.freq.packets);
}

chdr_rx_data_xport::~chdr_rx_data_xport()
{
    // Release recv_io before allowing members needed by callbacks be destroyed
    _recv_io.reset();

    // Disconnect the links
    _disconnect();
}

chdr_rx_data_xport::fc_params_t chdr_rx_data_xport::configure_sep(io_service::sptr io_srv,
    recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const chdr::chdr_packet_factory& pkt_factory,
    mgmt::mgmt_portal& mgmt_portal,
    const sep_id_pair_t& epids,
    const sw_buff_t pyld_buff_fmt,
    const sw_buff_t mdata_buff_fmt,
    const stream_buff_params_t& recv_capacity,
    const stream_buff_params_t& fc_freq,
    const stream_buff_params_t& fc_headroom,
    const bool lossy_xport,
    disconnect_callback_t disconnect)
{
    const sep_id_t remote_epid = epids.first;
    const sep_id_t local_epid  = epids.second;

    rx_flow_ctrl_sender fc_sender(pkt_factory, epids);
    chdr::chdr_packet_writer::uptr pkt = pkt_factory.make_generic();
    fc_sender.set_capacity(recv_capacity);
    chdr::strc_payload strc;

    auto recv_cb = [&pkt, local_epid, &strc](buff_t::uptr& buff,
                       recv_link_if* /*recv_link*/,
                       send_link_if* /*send_link*/) {
        pkt->refresh(buff->data());
        const auto header   = pkt->get_chdr_header();
        const auto dst_epid = header.get_dst_epid();

        if (dst_epid != local_epid) {
            return false;
        }

        const auto type = header.get_pkt_type();

        if (type != chdr::PKT_TYPE_STRC) {
            return false;
        }

        strc.deserialize(pkt->get_payload_const_ptr_as<uint64_t>(),
            pkt->get_payload_size() / sizeof(uint64_t),
            pkt->conv_to_host<uint64_t>());

        if (strc.op_code != chdr::STRC_INIT) {
            throw uhd::value_error("Unexpected opcode value in STRC packet.");
        }

        return true;
    };

    auto fc_cb = [&fc_sender](buff_t::uptr buff,
                     recv_link_if* recv_link,
                     send_link_if* send_link) {
        recv_link->release_recv_buff(std::move(buff));

        // Send a strs response to configure flow control on the sender. The
        // byte and packet counts are not important since they are reset by
        // the stream endpoint on receipt of this packet.
        fc_sender.send_strs(send_link, {0, 0});
    };

    // Create a temporary recv_io to receive the strc init
    auto recv_io = io_srv->make_recv_client(recv_link,
        1, // num_recv_frames
        recv_cb,
        send_link,
        1, // num_send_frames
        fc_cb);

    // Create a control transport with the rx data links to send mgmt packets
    // needed to setup the stream
    // Piggyback on frames from the recv_io_if
    auto ctrl_xport = uhd::rfnoc::chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        pkt_factory,
        local_epid,
        1, // num_send_frames
        1, // num_recv_frames
        disconnect);

    // Setup a route to the EPID
    // Note that this may be gratuitous--The endpoint may already have been set up
    mgmt_portal.setup_local_route(*ctrl_xport, remote_epid);

    // Initialize flow control - first, the management portal sends a stream
    // command containing its requested flow control frequency
    mgmt_portal.config_local_rx_stream_start(*ctrl_xport,
        remote_epid,
        lossy_xport,
        pyld_buff_fmt,
        mdata_buff_fmt,
        fc_freq,
        fc_headroom);

    // Now, release the buffer. In the flow control callback for the recv_io
    // (fc_cb above), we send a stream status containing the xport buffer
    // capacity.
    auto buff = recv_io->get_recv_buff(100);
    if (!buff) {
        UHD_LOG_THROW(uhd::runtime_error, "XPORT::RX_DATA_XPORT",
            "rx xport timed out getting a response from mgmt_portal");
    }
    recv_io->release_recv_buff(std::move(buff));

    // Finally, let the management portal know the setup is complete
    const bool fc_enabled = (fc_freq.bytes != 0) || (fc_freq.packets != 0);
    mgmt_portal.config_local_rx_stream_commit(
        *ctrl_xport, remote_epid, 0.2 /*default timeout*/, fc_enabled);

    // The flow control frequency requested is contained in the strc
    UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
        "Received strc init with fc freq"
            << " bytes=" << strc.num_bytes << ", packets=" << strc.num_pkts);

    fc_params_t fc_params;
    fc_params.buff_capacity = recv_capacity;
    fc_params.freq          = {strc.num_bytes, static_cast<uint32_t>(strc.num_pkts)};

    recv_io.reset();
    ctrl_xport.reset();

    return fc_params;
}
