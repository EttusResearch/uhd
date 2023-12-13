//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/rfnoc/rx_flow_ctrl_state.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

namespace mgmt {
class mgmt_portal;
}

namespace detail {

/*!
 * Utility class to send rx flow control responses
 */
class rx_flow_ctrl_sender
{
public:
    //! Constructor
    rx_flow_ctrl_sender(
        const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids);

    /*! configure buffer capacity
     * \param recv_capacity The buffer capacity of the receive link
     */
    void set_capacity(const stream_buff_params_t& recv_capacity);

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
    using uptr                  = std::unique_ptr<chdr_rx_data_xport>;
    using buff_t                = transport::frame_buff;
    using disconnect_callback_t = uhd::transport::disconnect_callback_t;

    //! Values extracted from received RX data packets
    struct packet_info_t
    {
        bool eob             = false;
        bool eov             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
        const void* payload  = nullptr;
    };

    //! Flow control parameters
    struct fc_params_t
    {
        stream_buff_params_t buff_capacity;
        stream_buff_params_t freq;
    };

    /*! Configure stream endpoint route and flow control
     *
     * \param io_srv The service that will schedule the xport I/O
     * \param recv_link The recv link, already attached to the I/O service
     * \param send_link The send link, already attached to the I/O service
     * \param pkt_factory Factory to create packets with the desired chdr_w and endianness
     * \param mgmt_portal Management portal to configure stream endpoint
     * \param epids Source and destination endpoint IDs
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param recv_capacity Total capacity of the recv link
     * \param fc_freq Frequency of flow control status messages
     * \param fc_headroom Headroom for flow control status messages
     * \param lossy_xport Whether the xport is lossy, for flow control configuration
     * \param xport_args Stream args
     * \param disconnect Callback function to disconnect the links
     * \return Parameters for xport flow control
     */
    static fc_params_t configure_sep(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const stream_buff_params_t& recv_capacity,
        const stream_buff_params_t& fc_freq,
        const stream_buff_params_t& fc_headroom,
        const bool lossy_xport,
        const uhd::device_addr_t& xport_args,
        disconnect_callback_t disconnect);

    /*! Constructor
     *
     * \param io_srv The service that will schedule the xport I/O
     * \param recv_link The recv link, already attached to the I/O service
     * \param send_link The send link, already attached to the I/O service
     * \param pkt_factory Factory to create packets with the desired chdr_w and endianness
     * \param epids Source and destination endpoint IDs
     * \param num_recv_frames Num frames to reserve from the recv link
     * \param fc_params Parameters for flow control
     * \param disconnect Callback function to disconnect the links
     */
    chdr_rx_data_xport(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const size_t num_recv_frames,
        const fc_params_t& fc_params,
        disconnect_callback_t disconnect);

    /*! Destructor
     */
    ~chdr_rx_data_xport();

    /*! Returns MTU for this transport in bytes
     *
     * MTU is the max size for CHDR packets, including headers. For most
     * applications, get_max_payload_size() is probably the more useful method.
     * Compare also noc_block_base::get_mtu().
     *
     * \return MTU in bytes
     */
    size_t get_mtu() const
    {
        return _mtu;
    }

    /*! Return the size of a CHDR packet header, in bytes.
     *
     * This helper function factors in the CHDR width for this transport.
     * Compare also noc_block_base::get_chdr_hdr_len().
     *
     * \returns the length of a CHDR header in bytes
     */
    size_t get_chdr_hdr_len() const
    {
        return _hdr_len;
    }

    /*! Returns maximum number of payload bytes
     *
     * This is smaller than the MTU. Compare also
     * noc_block_base::get_max_payload_size().
     *
     * \return maximum number of payload bytes
     */
    size_t get_max_payload_size() const
    {
        return _mtu - _hdr_len;
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
        _recv_packet_cb->refresh(buff->data());
        const auto header   = _recv_packet_cb->get_chdr_header();
        const auto dst_epid = header.get_dst_epid();

        if (dst_epid != _epid) {
            return false;
        }

        const auto type = header.get_pkt_type();
        // We need to round the packet size to the nearest multiple of a CHDR
        // width, because that's how the FPGA tracks bytes, and want to match
        // that behaviour.
        const auto packet_size_rounded = _round_pkt_size(header.get_length());

        if (type == chdr::PKT_TYPE_STRC) {
            chdr::strc_payload strc;
            strc.deserialize(_recv_packet_cb->get_payload_const_ptr_as<uint64_t>(),
                _recv_packet_cb->get_payload_size() / sizeof(uint64_t),
                _recv_packet_cb->conv_to_host<uint64_t>());

            const stream_buff_params_t strc_counts = {
                strc.num_bytes, static_cast<uint32_t>(strc.num_pkts)};

            if (strc.op_code == chdr::STRC_RESYNC) {
                // Resynchronize before updating fc_state, the strc payload
                // contains counts before the strc packet itself
                _fc_state.resynchronize(strc_counts);

                // Update state that we received a packet
                _fc_state.data_received(packet_size_rounded);

                recv_link->release_recv_buff(std::move(buff));
                buff = buff_t::uptr();
                _fc_state.xfer_done(packet_size_rounded);
                _send_fc_response(send_link);
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
            _fc_state.data_received(packet_size_rounded);

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
        _recv_packet_cb->refresh(buff->data());
        const auto header        = _recv_packet_cb->get_chdr_header();
        const size_t packet_size = _round_pkt_size(header.get_length());
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
     * Checks if the sequence number is out of sequence, increments sequence
     * number for next packet.
     *
     * \return true if a sequence error occurred
     */
    UHD_FORCE_INLINE bool _is_out_of_sequence(uint16_t seq_num)
    {
        const uint16_t expected_packet_count = _data_seq_num;
        _data_seq_num                        = seq_num + 1;

        if (expected_packet_count != seq_num) {
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
        info.eov           = header.get_eov();
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

    inline size_t _round_pkt_size(const size_t pkt_size_bytes)
    {
        return ((pkt_size_bytes + _chdr_w_bytes - 1) / _chdr_w_bytes) * _chdr_w_bytes;
    }

    // Interface to the I/O service
    transport::recv_io_if::sptr _recv_io;

    // Flow control state
    rx_flow_ctrl_state _fc_state;

    // MTU in bytes
    size_t _mtu = 0;

    // Size of CHDR headers
    size_t _hdr_len = 0;

    // Sequence number for data packets
    uint16_t _data_seq_num = 0;

    // Packet for received data
    chdr::chdr_packet_writer::uptr _recv_packet;

    // Packet for received data used in callbacks
    chdr::chdr_packet_writer::uptr _recv_packet_cb;

    // Handles sending of strs flow control response packets
    detail::rx_flow_ctrl_sender _fc_sender;

    // Local / Sink EPID
    sep_id_t _epid;

    //! The CHDR width in bytes.
    size_t _chdr_w_bytes;

    // Disconnect callback
    disconnect_callback_t _disconnect;
};

}} // namespace uhd::rfnoc
