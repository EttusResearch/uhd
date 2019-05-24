//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_CHDR_RX_DATA_XPORT_HPP
#define INCLUDED_LIBUHD_CHDR_RX_DATA_XPORT_HPP

#include <uhd/config.hpp>
#include <uhdlib/rfnoc/chdr_packet.hpp>
#include <uhdlib/rfnoc/chdr_types.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/rfnoc/rx_flow_ctrl_state.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

namespace detail {

/*!
 * Utility class to send rx flow control responses
 */
class rx_flow_ctrl_sender
{
public:
    //! Constructor
    rx_flow_ctrl_sender(
        const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids)
        : _dst_epid(sep_ids.first)
    {
        _fc_packet             = pkt_factory.make_strs();
        _fc_strs_pyld.src_epid = sep_ids.second;
    }

    /*! Configure buffer capacity
     * \param recv_capacity The buffer capacity of the receive link
     */
    void set_capacity(const stream_buff_params_t& recv_capacity)
    {
        _fc_strs_pyld.capacity_bytes = recv_capacity.bytes;
        _fc_strs_pyld.capacity_pkts  = recv_capacity.packets;
    }

    /*! Send a flow control response packet
     *
     * \param send_link the link to use to send the packet
     * \counts transfer counts for packet contents
     */
    void send_strs(transport::send_link_if* send_link, const stream_buff_params_t& counts)
    {
        auto buff = send_link->get_send_buff(0);
        if (!buff) {
            throw uhd::runtime_error("rx_flowctrl timed out getting a send buffer");
        }

        chdr::chdr_header header;
        header.set_seq_num(_fc_seq_num++);
        header.set_dst_epid(_dst_epid);

        chdr::strs_payload fc_payload(_fc_strs_pyld);
        fc_payload.xfer_count_bytes = counts.bytes;
        fc_payload.xfer_count_pkts  = counts.packets;

        _fc_packet->refresh(buff->data(), header, fc_payload);
        const size_t size = header.get_length();

        buff->set_packet_size(size);
        send_link->release_send_buff(std::move(buff));
    }

private:
    // Endpoint ID for recipient of flow control response
    const sep_id_t _dst_epid;

    // Packet for writing flow control info
    chdr::chdr_strs_packet::uptr _fc_packet;

    // Pre-configured strs payload to hold values that don't change
    chdr::strs_payload _fc_strs_pyld;

    // Sequence number for flow control packets
    uint16_t _fc_seq_num = 0;
};
} // namespace detail

/*!
 * Flow-controlled transport for RX chdr data
 *
 * This transport provides the streamer an interface to read RX data packets.
 * The transport implements flow control and sequence number checking.
 *
 * The transport uses I/O services to provide options for work scheduling. I/O
 * services allow the I/O work to be offloaded to a worker thread or to be
 * performed in the same thread as the streamer API calls.
 *
 * For an rx transport, the device sends data packets, and the host sends strs
 * packets letting the device know that buffer space in the host has been freed.
 * For lossy links, the device also sends strc packets to resynchronize the
 * transfer counts between host and device, to correct for any dropped packets
 * in the link.
 */
class chdr_rx_data_xport
{
public:
    using uptr   = std::unique_ptr<chdr_rx_data_xport>;
    using buff_t = transport::frame_buff;

    //! Values extracted from received RX data packets
    struct packet_info_t
    {
        bool eob             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
        const void* payload  = nullptr;
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
     * \param num_recv_frames Num frames to reserve from the recv link
     * \param recv_capacity Total capacity of the recv link
     * \param fc_freq Frequency of flow control status messages
     * \param fc_headroom Headroom for flow control status messages
     * \param lossy_xport Whether the xport is lossy, for flow control configuration
     */
    chdr_rx_data_xport(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const size_t num_recv_frames,
        const stream_buff_params_t& recv_capacity,
        const stream_buff_params_t& fc_freq,
        const stream_buff_params_t& fc_headroom,
        const bool lossy_xport)
        : _fc_state(epids), _fc_sender(pkt_factory, epids), _epid(epids.second)
    {
        const sep_addr_t remote_sep_addr = addrs.first;
        const sep_addr_t local_sep_addr  = addrs.second;
        const sep_id_t remote_epid       = epids.first;
        const sep_id_t local_epid        = epids.second;

        UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
            "Creating rx xport with local epid=" << local_epid
                                                 << ", remote epid=" << remote_epid);

        _recv_packet = pkt_factory.make_generic();
        _fc_sender.set_capacity(recv_capacity);

        // Calculate max payload size
        const size_t pyld_offset =
            _recv_packet->calculate_payload_offset(chdr::PKT_TYPE_DATA_WITH_TS);
        _max_payload_size = recv_link->get_recv_frame_size() - pyld_offset;

        // Make data transport
        auto recv_cb = [this](buff_t::uptr& buff,
                           transport::recv_link_if* recv_link,
                           transport::send_link_if* send_link) {
            return this->_recv_callback(buff, recv_link, send_link);
        };

        auto fc_cb = [this](buff_t::uptr buff,
                         transport::recv_link_if* recv_link,
                         transport::send_link_if* send_link) {
            this->_fc_callback(std::move(buff), recv_link, send_link);
        };

        // Needs just a single send frame for responses
        _recv_io = io_srv->make_recv_client(recv_link,
            num_recv_frames,
            recv_cb,
            send_link,
            /* num_send_frames*/ 1,
            fc_cb);

        // Create a control transport with the rx data links to send mgmt packets
        // needed to setup the stream
        // Piggyback on frames from the recv_io_if
        auto ctrl_xport = uhd::rfnoc::chdr_ctrl_xport::make(io_srv,
            send_link,
            recv_link,
            local_epid,
            0, // num_send_frames
            0); // num_recv_frames

        // Create new temporary management portal with the transports used for this stream
        // TODO: This is a bit excessive. Maybe we can pare down the functionality of the
        // portal just for route setup purposes. Whatever we do, we *must* use xport in it
        // though otherwise the transport will not behave correctly.
        auto data_mgmt_portal = uhd::rfnoc::mgmt::mgmt_portal::make(
            *ctrl_xport, pkt_factory, local_sep_addr, local_epid);

        // Setup a route to the EPID
        // Note that this may be gratuitous--The endpoint may already have been set up
        data_mgmt_portal->initialize_endpoint(*ctrl_xport, remote_sep_addr, remote_epid);
        data_mgmt_portal->setup_local_route(*ctrl_xport, remote_epid);

        // Initialize flow control - management portal sends a stream command
        // containing its requested flow control frequency, the rx transport
        // responds with a stream status containing its buffer capacity.
        data_mgmt_portal->config_local_rx_stream_start(*ctrl_xport,
            remote_epid,
            lossy_xport,
            pyld_buff_fmt,
            mdata_buff_fmt,
            fc_freq,
            fc_headroom);

        data_mgmt_portal->config_local_rx_stream_commit(*ctrl_xport, remote_epid);

        UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
            "Stream endpoint was configured with:"
                << std::endl
                << "capacity bytes=" << recv_capacity.bytes
                << ", packets=" << recv_capacity.packets << std::endl
                << "fc headroom bytes=" << fc_headroom.bytes
                << ", packets=" << fc_headroom.packets << std::endl
                << "fc frequency bytes=" << fc_freq.bytes
                << ", packets=" << fc_freq.packets);

        // We no longer need the control xport and mgmt_portal, release them so
        // the control xport is no longer connected to the I/O service.
        data_mgmt_portal.reset();
        ctrl_xport.reset();
    }

    /*! Returns maximum number payload bytes
     *
     * \return maximum payload bytes per packet
     */
    size_t get_max_payload_size() const
    {
        return _max_payload_size;
    }

    /*!
     * Gets an RX frame buffer containing a recv packet
     *
     * \param timeout_ms timeout in milliseconds
     * \return returns a tuple containing:
     * - a frame_buff, or null if timeout occurs
     * - info struct corresponding to the packet
     * - whether the packet was out of sequence
     */
    std::tuple<typename buff_t::uptr, packet_info_t, bool> get_recv_buff(
        const int32_t timeout_ms)
    {
        buff_t::uptr buff = _recv_io->get_recv_buff(timeout_ms);

        if (!buff) {
            return std::make_tuple(typename buff_t::uptr(), packet_info_t(), false);
        }

        auto info      = _read_data_packet_info(buff);
        bool seq_error = _is_out_of_sequence(std::get<1>(info));

        return std::make_tuple(std::move(buff), std::get<0>(info), seq_error);
    }

    /*!
     * Releases an RX frame buffer
     *
     * \param buff the frame buffer to release
     */
    void release_recv_buff(typename buff_t::uptr buff)
    {
        _recv_io->release_recv_buff(std::move(buff));
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
        transport::send_link_if* send_link)
    {
        _recv_packet->refresh(buff->data());
        const auto header      = _recv_packet->get_chdr_header();
        const auto type        = header.get_pkt_type();
        const auto dst_epid    = header.get_dst_epid();
        const auto packet_size = buff->packet_size();

        if (dst_epid != _epid) {
            return false;
        }

        if (type == chdr::PKT_TYPE_STRC) {
            chdr::strc_payload strc;
            strc.deserialize(_recv_packet->get_payload_const_ptr_as<uint64_t>(),
                _recv_packet->get_payload_size() / sizeof(uint64_t),
                _recv_packet->conv_to_host<uint64_t>());

            const stream_buff_params_t strc_counts = {
                strc.num_bytes, static_cast<uint32_t>(strc.num_pkts)};

            if (strc.op_code == chdr::STRC_RESYNC) {
                // Resynchronize before updating fc_state, the strc payload
                // contains counts before the strc packet itself
                _fc_state.resynchronize(strc_counts);

                // Update state that we received a packet
                _fc_state.data_received(packet_size);

                recv_link->release_recv_buff(std::move(buff));
                buff = buff_t::uptr();
                _fc_state.xfer_done(packet_size);
                _send_fc_response(send_link);
            } else if (strc.op_code == chdr::STRC_INIT) {
                _fc_state.initialize(
                    {strc.num_bytes, static_cast<uint32_t>(strc.num_pkts)});

                UHD_LOG_TRACE("XPORT::RX_DATA_XPORT",
                    "Received strc init with fc freq"
                        << " bytes=" << strc.num_bytes << ", packets=" << strc.num_pkts);

                // Make sure flow control was initialized
                assert(_fc_state.get_fc_freq().bytes > 0);
                assert(_fc_state.get_fc_freq().packets > 0);

                // Send a strs response to configure flow control on the sender
                _fc_sender.send_strs(send_link, _fc_state.get_xfer_counts());

                // Reset counts, since mgmt_portal will do it to FPGA
                _fc_state.reset_counts();

                recv_link->release_recv_buff(std::move(buff));
                buff = buff_t::uptr();
            } else {
                throw uhd::value_error("Unexpected opcode value in STRC packet.");
            }

            // For stream commands, we return true (packet was destined to this
            // client) but release the buffer. The I/O service won't queue this
            // packet in the recv_io_if.
            return true;

        } else if (type == chdr::PKT_TYPE_DATA_NO_TS
                   || type == chdr::PKT_TYPE_DATA_WITH_TS) {
            // Update state that we received a packet
            _fc_state.data_received(packet_size);

            // If this is a data packet, just claim it by returning true. The
            // I/O service will queue this packet in the recv_io_if.
            return true;

        } else {
            return false;
        }
    }

    /*!
     * Flow control callback for I/O service
     *
     * The I/O service invokes this callback when a packet needs to be released
     * to the recv link.
     *
     * \param buff the frame buffer containing the packet data
     * \param recv_link the recv link to which to release the buffer
     * \param send_link the send link for flow control messages
     */
    void _fc_callback(buff_t::uptr buff,
        transport::recv_link_if* recv_link,
        transport::send_link_if* send_link)
    {
        const size_t packet_size = buff->packet_size();
        recv_link->release_recv_buff(std::move(buff));
        _fc_state.xfer_done(packet_size);
        _send_fc_response(send_link);
    }

    /*!
     * Sends a flow control response packet if necessary.
     *
     * \param send_link the send link for flow control messages
     */
    void _send_fc_response(transport::send_link_if* send_link)
    {
        if (_fc_state.fc_resp_due()) {
            _fc_sender.send_strs(send_link, _fc_state.get_xfer_counts());
            _fc_state.fc_resp_sent();
        }
    }

    /*!
     * Checks if the sequence number is out of sequence, prints 'D' if it is
     * and returns result of check.
     *
     * \return true if a sequence error occurred
     */
    UHD_FORCE_INLINE bool _is_out_of_sequence(uint16_t seq_num)
    {
        const uint16_t expected_packet_count = _data_seq_num;
        _data_seq_num                        = seq_num + 1;

        if (expected_packet_count != seq_num) {
            UHD_LOG_FASTPATH("D");
            return true;
        }
        return false;
    }

    /*!
     * Reads packet header and returns information in a struct.
     *
     * \return a tuple containing the packet info and packet sequence number
     */
    std::tuple<packet_info_t, uint16_t> _read_data_packet_info(buff_t::uptr& buff)
    {
        const void* data = buff->data();
        _recv_packet->refresh(data);
        const auto header        = _recv_packet->get_chdr_header();
        const auto optional_time = _recv_packet->get_timestamp();

        packet_info_t info;
        info.eob           = header.get_eob();
        info.has_tsf       = optional_time.is_initialized();
        info.tsf           = optional_time ? *optional_time : 0;
        info.payload_bytes = _recv_packet->get_payload_size();
        info.payload       = _recv_packet->get_payload_const_ptr();

        const uint8_t* pkt_end =
            reinterpret_cast<uint8_t*>(buff->data()) + buff->packet_size();
        const size_t pyld_pkt_len =
            pkt_end - reinterpret_cast<const uint8_t*>(info.payload);

        if (pyld_pkt_len < info.payload_bytes) {
            _recv_io->release_recv_buff(std::move(buff));
            throw uhd::value_error("Bad CHDR header or invalid packet length.");
        }

        return std::make_tuple(info, header.get_seq_num());
    }

    // Interface to the I/O service
    transport::recv_io_if::sptr _recv_io;

    // Flow control state
    rx_flow_ctrl_state _fc_state;

    // Maximum data payload in bytes
    size_t _max_payload_size = 0;

    // Sequence number for data packets
    uint16_t _data_seq_num = 0;

    // Packet for received data
    chdr::chdr_packet::uptr _recv_packet;

    // Handles sending of strs flow control response packets
    detail::rx_flow_ctrl_sender _fc_sender;

    // Local / Sink EPID
    sep_id_t _epid;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_CHDR_RX_DATA_XPORT_HPP */
