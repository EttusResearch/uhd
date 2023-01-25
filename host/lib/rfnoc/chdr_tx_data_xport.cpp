//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_tx_data_xport.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::detail;
using namespace uhd::transport;

tx_flow_ctrl_sender::tx_flow_ctrl_sender(
    const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids)
    : _dst_epid(sep_ids.second)
{
    _fc_packet             = pkt_factory.make_strc();
    _fc_strc_pyld.src_epid = sep_ids.first;
    _fc_strc_pyld.op_code  = chdr::STRC_RESYNC;
}

chdr_tx_data_xport::chdr_tx_data_xport(uhd::transport::io_service::sptr io_srv,
    uhd::transport::recv_link_if::sptr recv_link,
    uhd::transport::send_link_if::sptr send_link,
    const chdr::chdr_packet_factory& pkt_factory,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const size_t num_send_frames,
    const fc_params_t fc_params,
    disconnect_callback_t disconnect)
    : _fc_state(fc_params.buff_capacity)
    , _mtu(send_link->get_send_frame_size())
    , _fc_sender(pkt_factory, epids)
    , _epid(epids.first)
    , _chdr_w_bytes(chdr_w_to_bits(pkt_factory.get_chdr_w()) / 8)
    , _frame_size(send_link->get_send_frame_size())
    , _disconnect(disconnect)
{
    UHD_LOG_TRACE("XPORT::TX_DATA_XPORT",
        "Creating tx xport with local epid=" << epids.first
                                             << ", remote epid=" << epids.second);

    _send_header.set_dst_epid(epids.second);
    _send_packet = pkt_factory.make_generic();
    _recv_packet = pkt_factory.make_generic();

    // Calculate header length
    _hdr_len = _send_packet->calculate_payload_offset(chdr::PKT_TYPE_DATA_WITH_TS);
    UHD_ASSERT_THROW(_hdr_len);

    // Now create the send I/O we will use for data
    auto send_cb = [this](buff_t::uptr buff, transport::send_link_if* send_link) {
        this->_send_callback(std::move(buff), send_link);
    };

    auto recv_cb = [this](buff_t::uptr& buff,
                       transport::recv_link_if* recv_link,
                       transport::send_link_if* send_link) {
        return this->_recv_callback(buff, recv_link, send_link);
    };

    auto fc_cb = [this](size_t num_bytes) { return this->_fc_callback(num_bytes); };

    // Needs just a single recv frame for strs packets
    _send_io = io_srv->make_send_client(send_link,
        num_send_frames,
        send_cb,
        recv_link,
        /* num_recv_frames */ 1,
        recv_cb,
        fc_cb);
}

chdr_tx_data_xport::~chdr_tx_data_xport()
{
    // Release send_io before allowing members needed by callbacks be destroyed
    _send_io.reset();

    // Disconnect the transport
    _disconnect();
}

/*
 * To configure flow control, we need to send an init strc packet, then
 * receive a strs containing the stream endpoint ingress buffer size. We
 * then repeat this (now that we know the buffer size) to configure the flow
 * control frequency. To avoid having this logic within the data packet
 * processing flow, we use temporary send and recv I/O instances with
 * simple callbacks here.
 */
static chdr_tx_data_xport::fc_params_t configure_flow_ctrl(io_service::sptr io_srv,
    recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const chdr::chdr_packet_factory& pkt_factory,
    const sep_id_pair_t epids,
    const double fc_freq_ratio,
    const double fc_headroom_ratio)
{
    chdr::chdr_strc_packet::uptr strc_packet   = pkt_factory.make_strc();
    chdr::chdr_packet_writer::uptr recv_packet = pkt_factory.make_generic();

    // No flow control at initialization, just release all send buffs
    auto send_cb = [](frame_buff::uptr buff, send_link_if* send_link) {
        send_link->release_send_buff(std::move(buff));
    };

    // For recv, just queue strs packets for recv_io to read
    auto recv_cb = [&recv_packet, epids](frame_buff::uptr& buff,
                       recv_link_if* /*recv_link*/,
                       send_link_if* /*send_link*/) {
        recv_packet->refresh(buff->data());
        const auto header   = recv_packet->get_chdr_header();
        const auto type     = header.get_pkt_type();
        const auto dst_epid = header.get_dst_epid();

        return (dst_epid == epids.first && type == chdr::PKT_TYPE_STRS);
    };

    // No flow control at initialization, just release all recv buffs
    auto fc_cb =
        [](frame_buff::uptr buff, recv_link_if* recv_link, send_link_if* /*send_link*/) {
            recv_link->release_recv_buff(std::move(buff));
        };

    auto send_io = io_srv->make_send_client(send_link,
        1, // num_send_frames
        send_cb,
        nullptr,
        0, // num_recv_frames
        nullptr,
        nullptr);

    auto recv_io = io_srv->make_recv_client(recv_link,
        1, // num_recv_frames
        recv_cb,
        nullptr,
        0, // num_send_frames
        fc_cb);

    // Function to send a strc init
    auto send_strc_init = [&send_io, epids, &strc_packet](
                              const stream_buff_params_t fc_freq = {0, 0}) {
        frame_buff::uptr buff = send_io->get_send_buff(0);

        if (!buff) {
            UHD_LOG_THROW(uhd::runtime_error, "XPORT::TX_DATA_XPORT",
                "tx xport timed out getting a response from mgmt_portal");
        }

        chdr::chdr_header header;
        header.set_seq_num(0);
        header.set_dst_epid(epids.second);

        chdr::strc_payload strc_pyld;
        strc_pyld.src_epid  = epids.first;
        strc_pyld.op_code   = chdr::STRC_INIT;
        strc_pyld.num_bytes = fc_freq.bytes;
        strc_pyld.num_pkts  = fc_freq.packets;
        strc_packet->refresh(buff->data(), header, strc_pyld);

        const size_t size = header.get_length();
        buff->set_packet_size(size);
        send_io->release_send_buff(std::move(buff));
    };

    // Function to receive a strs, returns buffer capacity
    auto recv_strs = [&recv_io, &recv_packet]() -> stream_buff_params_t {
        frame_buff::uptr buff = recv_io->get_recv_buff(200);

        if (!buff) {
            throw uhd::runtime_error(
                "tx xport timed out wating for a strs packet during initialization");
        }

        recv_packet->refresh(buff->data());
        UHD_ASSERT_THROW(
            recv_packet->get_chdr_header().get_pkt_type() == chdr::PKT_TYPE_STRS);
        chdr::strs_payload strs;
        strs.deserialize(recv_packet->get_payload_const_ptr_as<uint64_t>(),
            recv_packet->get_payload_size() / sizeof(uint64_t),
            recv_packet->conv_to_host<uint64_t>());

        recv_io->release_recv_buff(std::move(buff));

        return {strs.capacity_bytes, static_cast<uint32_t>(strs.capacity_pkts)};
    };

    // Send a strc init to get the buffer size
    send_strc_init();
    stream_buff_params_t capacity = recv_strs();

    UHD_LOG_TRACE("XPORT::TX_DATA_XPORT",
        "Received strs initializing buffer capacity to " << capacity.bytes << " bytes");

    // Calculate the requested fc_freq parameters
    stream_buff_params_t fc_freq = {
        static_cast<uint64_t>(std::ceil(double(capacity.bytes) * fc_freq_ratio)),
        static_cast<uint32_t>(std::ceil(double(capacity.packets) * fc_freq_ratio))};

    const size_t headroom_bytes =
        static_cast<uint64_t>(std::ceil(double(capacity.bytes) * fc_headroom_ratio));
    const size_t headroom_packets =
        static_cast<uint32_t>(std::ceil(double(capacity.packets) * fc_headroom_ratio));

    fc_freq.bytes -= headroom_bytes;
    fc_freq.packets -= headroom_packets;

    // Send a strc init to configure fc freq
    send_strc_init(fc_freq);
    recv_strs();

    // Release temporary I/O service interfaces to disconnect from it
    send_io.reset();
    recv_io.reset();

    return {capacity};
}

chdr_tx_data_xport::fc_params_t chdr_tx_data_xport::configure_sep(io_service::sptr io_srv,
    recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const chdr::chdr_packet_factory& pkt_factory,
    mgmt::mgmt_portal& mgmt_portal,
    const sep_id_pair_t& epids,
    const sw_buff_t pyld_buff_fmt,
    const sw_buff_t mdata_buff_fmt,
    const double fc_freq_ratio,
    const double fc_headroom_ratio,
    disconnect_callback_t disconnect)
{
    const sep_id_t remote_epid = epids.second;
    const sep_id_t local_epid  = epids.first;

    // Create a control transport with the tx data links to send mgmt packets
    // needed to setup the stream. Only need one frame for this.
    auto ctrl_xport = chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        pkt_factory,
        local_epid,
        1, // num_send_frames
        1, // num_recv_frames
        disconnect);

    // Setup a route to the EPID
    mgmt_portal.setup_local_route(*ctrl_xport, remote_epid);

    mgmt_portal.config_local_tx_stream(
        *ctrl_xport, remote_epid, pyld_buff_fmt, mdata_buff_fmt);

    return configure_flow_ctrl(io_srv,
        recv_link,
        send_link,
        pkt_factory,
        epids,
        fc_freq_ratio,
        fc_headroom_ratio);
}
