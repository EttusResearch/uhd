//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_CHDR_TX_DATA_XPORT_HPP
#define INCLUDED_LIBUHD_CHDR_TX_DATA_XPORT_HPP

#include <uhdlib/rfnoc/chdr_packet.hpp>
#include <uhdlib/rfnoc/chdr_types.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/rfnoc/tx_flow_ctrl_state.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

namespace detail {

/*!
 * Utility class to send tx flow control messages
 */
class tx_flow_ctrl_sender
{
public:
    //! Constructor
    tx_flow_ctrl_sender(
        const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids)
        : _dst_epid(sep_ids.second)
    {
        _fc_packet             = pkt_factory.make_strc();
        _fc_strc_pyld.src_epid = sep_ids.first;
        _fc_strc_pyld.op_code  = chdr::STRC_RESYNC;
    }

    /*!
     * Sends a flow control resync packet
     *
     * Sends a strc packet with the resync opcode to make the device transfer
     * counts match those of the host, to correct for dropped packets.
     *
     * \param send_link the link to use to send the packet
     * \counts transfer counts for packet contents
     */
    size_t send_strc_resync(
        transport::send_link_if* send_link, const stream_buff_params_t& counts)
    {
        auto buff = send_link->get_send_buff(0);
        if (!buff) {
            throw uhd::runtime_error("tx_flowctrl timed out getting a send buffer");
        }

        chdr::chdr_header header;
        header.set_seq_num(_fc_seq_num++);
        header.set_dst_epid(_dst_epid);

        chdr::strc_payload fc_payload(_fc_strc_pyld);
        fc_payload.num_bytes = counts.bytes;
        fc_payload.num_pkts  = counts.packets;

        _fc_packet->refresh(buff->data(), header, fc_payload);
        const size_t size = header.get_length();

        buff->set_packet_size(size);
        send_link->release_send_buff(std::move(buff));
        return size;
    }

private:
    // Endpoint ID for recipient of flow control response
    const sep_id_t _dst_epid;

    // Packet for writing flow control info
    chdr::chdr_strc_packet::uptr _fc_packet;

    // Pre-configured strc payload to hold values that don't change
    chdr::strc_payload _fc_strc_pyld;

    // Sequence number for flow control packets
    uint16_t _fc_seq_num = 0;
};
} // namespace detail

/*!
 * Flow-controlled transport for TX chdr data
 *
 * This transport provides the streamer an interface to send TX data packets.
 * The transport implements flow control and keeps track of sequence numbers.
 *
 * The transport uses I/O services to provide options for work scheduling. I/O
 * services allow the I/O work to be offloaded to a worker thread or to be
 * performed in the same thread as the streamer API calls.
 *
 * For a tx transport, the host sends data packets, and the device sends strs
 * packets letting the host know that buffer space in the device stream endpoint
 * has been freed. For lossy links, the host also sends strc packets to
 * resynchronize the transfer counts between host and device, to correct for
 * any dropped packets in the link.
 */
class chdr_tx_data_xport
{
public:
    using uptr   = std::unique_ptr<chdr_tx_data_xport>;
    using buff_t = transport::frame_buff;

    //! Information about data packet
    struct packet_info_t
    {
        bool eob             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
    };

    /*! Constructor
     *
     * \param io_srv The service that will schedule the xport I/O
     * \param recv_link The recv link, already attached to the I/O service
     * \param send_link The send link, already attached to the I/O service
     * \param pkt_factory Factory to create packets with the desired chdr_w and endianness
     * \param addrs Source and destination addresses
     * \param epids Source and destination endpoint IDs
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param num_send_frames Num frames to reserve from the send link
     * \param fc_freq_ratio Ratio to use to configure the device fc frequency
     * \param fc_headroom_ratio Ratio to use to configure the device fc headroom
     */
    chdr_tx_data_xport(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const size_t num_send_frames,
        const double fc_freq_ratio,
        const double fc_headroom_ratio)
        : _fc_sender(pkt_factory, epids), _epid(epids.first)
    {
        const sep_addr_t remote_sep_addr = addrs.second;
        const sep_addr_t local_sep_addr  = addrs.first;
        const sep_id_t remote_epid       = epids.second;
        const sep_id_t local_epid        = epids.first;

        UHD_LOG_TRACE("XPORT::TX_DATA_XPORT",
            "Creating tx xport with local epid=" << local_epid
                                                 << ", remote epid=" << remote_epid);

        _send_header.set_dst_epid(epids.second);
        _send_packet = pkt_factory.make_generic();
        _recv_packet = pkt_factory.make_generic();

        // Calculate max payload size
        const size_t pyld_offset =
            _send_packet->calculate_payload_offset(chdr::PKT_TYPE_DATA_WITH_TS);
        _max_payload_size = send_link->get_send_frame_size() - pyld_offset;

        _configure_sep(io_srv,
            recv_link,
            send_link,
            pkt_factory,
            local_sep_addr,
            local_epid,
            remote_sep_addr,
            remote_epid,
            pyld_buff_fmt,
            mdata_buff_fmt);

        _initialize_flow_ctrl(io_srv,
            recv_link,
            send_link,
            pkt_factory,
            epids,
            fc_freq_ratio,
            fc_headroom_ratio);

        // Now create the send I/O we will use for data
        auto send_cb = [this](buff_t::uptr& buff, transport::send_link_if* send_link) {
            this->_send_callback(buff, send_link);
        };

        auto recv_cb = [this](buff_t::uptr& buff,
                           transport::recv_link_if* recv_link,
                           transport::send_link_if* send_link) {
            return this->_recv_callback(buff, recv_link, send_link);
        };

        // Needs just a single recv frame for strs packets
        _send_io = io_srv->make_send_client(send_link,
            num_send_frames,
            send_cb,
            recv_link,
            /* num_recv_frames */ 1,
            recv_cb);
    }

    /*! Returns maximum number of payload bytes
     *
     * \return maximum number of payload bytes
     */
    size_t get_max_payload_size() const
    {
        return _max_payload_size;
    }

    /*!
     * Gets a TX frame buffer
     *
     * \param timeout_ms timeout in milliseconds
     * \return the frame buffer, or nullptr if timeout occurs
     */
    buff_t::uptr get_send_buff(const int32_t timeout_ms)
    {
        return _send_io->get_send_buff(timeout_ms);
    }

    /*!
     * Sends a TX data packet
     *
     * \param buff the frame buffer containing the packet to send
     */
    void release_send_buff(buff_t::uptr buff)
    {
        _send_io->release_send_buff(std::move(buff));
    }

    /*!
     * Writes header into frame buffer and returns payload pointer
     *
     * \param buff Frame buffer to write header into
     * \param info Information to include in the header
     * \return A pointer to the payload data area and the packet size in bytes
     */
    std::pair<void*, size_t> write_packet_header(buff_t::uptr& buff,
        const packet_info_t& info)
    {
        uint64_t tsf = 0;

        if (info.has_tsf) {
            _send_header.set_pkt_type(chdr::PKT_TYPE_DATA_WITH_TS);
            tsf = info.tsf;
        } else {
            _send_header.set_pkt_type(chdr::PKT_TYPE_DATA_NO_TS);
        }

        _send_header.set_eob(info.eob);
        _send_header.set_seq_num(_data_seq_num++);

        _send_packet->refresh(buff->data(), _send_header, tsf);
        _send_packet->update_payload_size(info.payload_bytes);

        return std::make_pair(
            _send_packet->get_payload_ptr(),
            _send_packet->get_chdr_header().get_length());
    }

private:
    /*!
     * Recv callback for I/O service
     *
     * The I/O service invokes this callback when it reads a packet from the
     * recv link.
     *
     * \param buff the frame buffer containing the packet data
     * \param recv_link the recv link from which buff was read
     * \param send_link the send link for flow control messages
     */
    bool _recv_callback(buff_t::uptr& buff,
        transport::recv_link_if* recv_link,
        transport::send_link_if* /*send_link*/)
    {
        _recv_packet->refresh(buff->data());
        const auto header   = _recv_packet->get_chdr_header();
        const auto type     = header.get_pkt_type();
        const auto dst_epid = header.get_dst_epid();

        if (dst_epid != _epid) {
            return false;
        }

        if (type == chdr::PKT_TYPE_STRS) {
            chdr::strs_payload strs;
            strs.deserialize(_recv_packet->get_payload_const_ptr_as<uint64_t>(),
                _recv_packet->get_payload_size() / sizeof(uint64_t),
                _recv_packet->conv_to_host<uint64_t>());

            _fc_state.update_dest_recv_count({strs.xfer_count_bytes,
                static_cast<uint32_t>(strs.xfer_count_pkts)});

            // TODO: check strs status here and push into async msg queue

            // Packet belongs to this transport, release buff and return true
            recv_link->release_recv_buff(std::move(buff));
            buff = nullptr;
            return true;
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    /*!
     * Send callback for I/O service
     *
     * The I/O service invokes this callback when it is requested to release
     * a send buffer to the send link.
     *
     * \param buff the frame buffer to release
     * \param send_link the send link for flow control messages
     */
    void _send_callback(buff_t::uptr& buff, transport::send_link_if* send_link)
    {
        const size_t packet_size = buff->packet_size();

        if (_fc_state.dest_has_space(packet_size)) {
            send_link->release_send_buff(std::move(buff));
            buff = nullptr;

            _fc_state.data_sent(packet_size);

            if (_fc_state.get_fc_resync_req_pending()
                && _fc_state.dest_has_space(chdr::strc_payload::PACKET_SIZE)) {
                const auto& xfer_counts = _fc_state.get_xfer_counts();
                const size_t strc_size =
                    _fc_sender.send_strc_resync(send_link, xfer_counts);
                _fc_state.clear_fc_resync_req_pending();
                _fc_state.data_sent(strc_size);
            }
        }
    }

    /*!
     * Configures the stream endpoint using mgmt_portal
     */
    void _configure_sep(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const uhd::rfnoc::sep_addr_t& local_sep_addr,
        const uhd::rfnoc::sep_id_t& local_epid,
        const uhd::rfnoc::sep_addr_t& remote_sep_addr,
        const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt)
    {
        // Create a control transport with the tx data links to send mgmt packets
        // needed to setup the stream. Only need one frame for this.
        auto ctrl_xport = uhd::rfnoc::chdr_ctrl_xport::make(io_srv,
            send_link,
            recv_link,
            local_epid,
            1, // num_send_frames
            1); // num_recv_frames

        // Create new temporary management portal with the transports used for this stream
        // TODO: This is a bit excessive. Maybe we can pare down the functionality of the
        // portal just for route setup purposes. Whatever we do, we *must* use xport in it
        // though otherwise the transport will not behave correctly.
        auto data_mgmt_portal = uhd::rfnoc::mgmt::mgmt_portal::make(
            *ctrl_xport, pkt_factory, local_sep_addr, local_epid);

        // Setup a route to the EPID
        data_mgmt_portal->initialize_endpoint(*ctrl_xport, remote_sep_addr, remote_epid);
        data_mgmt_portal->setup_local_route(*ctrl_xport, remote_epid);

        data_mgmt_portal->config_local_tx_stream(
            *ctrl_xport, remote_epid, pyld_buff_fmt, mdata_buff_fmt);

        // We no longer need the control xport and mgmt_portal, release them so
        // the control xport is no longer connected to the I/O service.
        data_mgmt_portal.reset();
        ctrl_xport.reset();
    }

    /*!
     * Initializes flow control
     *
     * To initialize flow control, we need to send an init strc packet, then
     * receive a strs containing the stream endpoint ingress buffer size. We
     * then repeat this (now that we know the buffer size) to configure the flow
     * control frequency. To avoid having this logic within the data packet
     * processing flow, we use temporary send and recv I/O instances with
     * simple callbacks here.
     */
    void _initialize_flow_ctrl(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const sep_id_pair_t sep_ids,
        const double fc_freq_ratio,
        const double fc_headroom_ratio)
    {
        // No flow control at initialization, just release all send buffs
        auto send_cb = [this](buff_t::uptr& buff, transport::send_link_if* send_link) {
            send_link->release_send_buff(std::move(buff));
            buff = nullptr;
        };

        // For recv, just queue strs packets for recv_io to read
        auto recv_cb = [this](buff_t::uptr& buff,
                           transport::recv_link_if* /*recv_link*/,
                           transport::send_link_if* /*send_link*/) {
            _recv_packet->refresh(buff->data());
            const auto header   = _recv_packet->get_chdr_header();
            const auto type     = header.get_pkt_type();
            const auto dst_epid = header.get_dst_epid();

            return (dst_epid == _epid && type == chdr::PKT_TYPE_STRS);
        };

        // No flow control at initialization, just release all recv buffs
        auto fc_cb = [this](buff_t::uptr buff,
                         transport::recv_link_if* recv_link,
                         transport::send_link_if* /*send_link*/) {
            recv_link->release_recv_buff(std::move(buff));
        };

        auto send_io = io_srv->make_send_client(send_link,
            1, // num_send_frames
            send_cb,
            nullptr,
            0, // num_recv_frames
            nullptr);

        auto recv_io = io_srv->make_recv_client(recv_link,
            1, // num_recv_frames
            recv_cb,
            nullptr,
            0, // num_send_frames
            fc_cb);

        chdr::chdr_strc_packet::uptr strc_packet = pkt_factory.make_strc();
        chdr::chdr_packet::uptr& recv_packet     = _recv_packet;

        // Function to send a strc init
        auto send_strc_init = [&send_io, sep_ids, &strc_packet](
                                  const stream_buff_params_t fc_freq = {0, 0}) {
            transport::frame_buff::uptr buff = send_io->get_send_buff(0);

            if (!buff) {
                throw uhd::runtime_error(
                    "tx xport timed out getting a send buffer for strc init");
            }

            chdr::chdr_header header;
            header.set_seq_num(0);
            header.set_dst_epid(sep_ids.second);

            chdr::strc_payload strc_pyld;
            strc_pyld.src_epid  = sep_ids.first;
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
            transport::frame_buff::uptr buff = recv_io->get_recv_buff(200);

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

            return {strs.capacity_bytes,
                static_cast<uint32_t>(strs.capacity_pkts)};
        };

        // Send a strc init to get the buffer size
        send_strc_init();
        stream_buff_params_t capacity = recv_strs();
        _fc_state.set_dest_capacity(capacity);

        UHD_LOG_TRACE("XPORT::TX_DATA_XPORT",
            "Received strs initializing buffer capacity to "
            << capacity.bytes << " bytes");

        // Calculate the requested fc_freq parameters
        uhd::rfnoc::stream_buff_params_t fc_freq = {
            static_cast<uint64_t>(std::ceil(double(capacity.bytes) * fc_freq_ratio)),
            static_cast<uint32_t>(
                std::ceil(double(capacity.packets) * fc_freq_ratio))};

        const size_t headroom_bytes =
            static_cast<uint64_t>(std::ceil(double(capacity.bytes) * fc_headroom_ratio));
        const size_t headroom_packets = static_cast<uint32_t>(
            std::ceil(double(capacity.packets) * fc_headroom_ratio));

        fc_freq.bytes -= headroom_bytes;
        fc_freq.packets -= headroom_packets;

        // Send a strc init to configure fc freq
        send_strc_init(fc_freq);
        recv_strs();

        // Release temporary I/O service interfaces to disconnect from it
        send_io.reset();
        recv_io.reset();
    }

    // Interface to the I/O service
    transport::send_io_if::sptr _send_io;

    // Flow control state
    tx_flow_ctrl_state _fc_state;

    // Maximum data payload in bytes
    size_t _max_payload_size = 0;

    // Sequence number for data packets
    uint16_t _data_seq_num = 0;

    // Header to write into send packets
    chdr::chdr_header _send_header;

    // Packet for send data
    chdr::chdr_packet::uptr _send_packet;

    // Packet to receive strs messages
    chdr::chdr_packet::uptr _recv_packet;

    // Handles sending of strc flow control ack packets
    detail::tx_flow_ctrl_sender _fc_sender;

    // Local / Source EPID
    sep_id_t _epid;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_CHDR_TX_DATA_XPORT_HPP */
