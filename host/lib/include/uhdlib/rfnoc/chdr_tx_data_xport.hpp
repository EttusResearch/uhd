//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/rfnoc/tx_flow_ctrl_state.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

namespace mgmt {
class mgmt_portal;
}

namespace detail {

/*!
 * Utility class to send tx flow control messages
 */
class tx_flow_ctrl_sender
{
public:
    //! Constructor
    tx_flow_ctrl_sender(
        const chdr::chdr_packet_factory& pkt_factory, const sep_id_pair_t sep_ids);

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
    using enqueue_async_msg_fn_t =
        std::function<void(async_metadata_t::event_code_t, bool, uint64_t)>;
    using disconnect_callback_t = uhd::transport::disconnect_callback_t;

    //! Information about data packet
    struct packet_info_t
    {
        bool eob             = false;
        bool eov             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
    };

    //! Flow control parameters
    struct fc_params_t
    {
        stream_buff_params_t buff_capacity;
    };

    /*! Configure route to the sep and flow control
     *
     * \param io_srv The I/O service to be used with the transport
     * \param recv_link The recv link, already attached to the I/O service
     * \param send_link The send link, already attached to the I/O service
     * \param pkt_factory Factory to create packets with the desired chdr_w and endianness
     * \param mgmt_portal Management portal to configure stream endpoint
     * \param epids Source and destination endpoint IDs
     * \param pyld_buff_fmt Datatype of SW buffer that holds the data payload
     * \param mdata_buff_fmt Datatype of SW buffer that holds the data metadata
     * \param fc_freq_ratio Ratio to use to configure the device fc frequency
     * \param fc_headroom_ratio Ratio to use to configure the device fc headroom
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
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        disconnect_callback_t disconnect);

    /*! Constructor
     *
     * \param io_srv The service that will schedule the xport I/O
     * \param recv_link The recv link, already attached to the I/O service
     * \param send_link The send link, already attached to the I/O service
     * \param pkt_factory Factory to create packets with the desired chdr_w and endianness
     * \param epids Source and destination endpoint IDs
     * \param num_send_frames Num frames to reserve from the send link
     * \param fc_params Parameters for flow control
     * \param disconnect Callback function to disconnect the links
     */
    chdr_tx_data_xport(uhd::transport::io_service::sptr io_srv,
        uhd::transport::recv_link_if::sptr recv_link,
        uhd::transport::send_link_if::sptr send_link,
        const chdr::chdr_packet_factory& pkt_factory,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const size_t num_send_frames,
        const fc_params_t fc_params,
        disconnect_callback_t disconnect);

    /*! Destructor
     */
    ~chdr_tx_data_xport();

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
     * Gets a TX frame buffer
     *
     * \param timeout_ms timeout in milliseconds
     * \return the frame buffer, or nullptr if timeout occurs
     */
    buff_t::uptr get_send_buff(const int32_t timeout_ms)
    {
        if (_send_io->wait_for_dest_ready(_frame_size, timeout_ms)) {
            return _send_io->get_send_buff(timeout_ms);
        } else {
            return nullptr;
        }
    }

    /*!
     * Configure a function to call to enqueue async msgs
     *
     * \param fn Function to enqueue async messages
     */
    void set_enqueue_async_msg_fn(enqueue_async_msg_fn_t fn)
    {
        _enqueue_async_msg = fn;
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
    std::pair<void*, size_t> write_packet_header(
        buff_t::uptr& buff, const packet_info_t& info)
    {
        uint64_t tsf = 0;

        if (info.has_tsf) {
            _send_header.set_pkt_type(chdr::PKT_TYPE_DATA_WITH_TS);
            tsf = info.tsf;
        } else {
            _send_header.set_pkt_type(chdr::PKT_TYPE_DATA_NO_TS);
        }

        _send_header.set_eob(info.eob);
        _send_header.set_eov(info.eov);
        _send_header.set_seq_num(_data_seq_num++);

        _send_packet->refresh(buff->data(), _send_header, tsf);
        _send_packet->update_payload_size(info.payload_bytes);

        return std::make_pair(_send_packet->get_payload_ptr(),
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

            _fc_state.update_dest_recv_count(
                {strs.xfer_count_bytes, static_cast<uint32_t>(strs.xfer_count_pkts)});

            if (strs.status != chdr::STRS_OKAY) {
                switch (strs.status) {
                    case chdr::STRS_SEQERR:
                        UHD_LOG_FASTPATH("S");
                        if (_enqueue_async_msg) {
                            _enqueue_async_msg(
                                async_metadata_t::EVENT_CODE_SEQ_ERROR, false, 0);
                        }
                        break;
                    case chdr::STRS_DATAERR:
                        UHD_LOG_WARNING(
                            "XPORT::TX_DATA_XPORT", "Received data error in tx stream!");
                        break;
                    case chdr::STRS_RTERR:
                        UHD_LOG_WARNING("XPORT::TX_DATA_XPORT",
                            "Received routing error in tx stream!");
                        break;
                    case chdr::STRS_CMDERR:
                        UHD_LOG_WARNING("XPORT::TX_DATA_XPORT",
                            "Received command error in tx stream!");
                        break;
                    default:
                        break;
                }
            }

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
    void _send_callback(buff_t::uptr buff, transport::send_link_if* send_link)
    {
        // If the packet size is not a multiple of the word size, then we will
        // still occupy an integer multiple of word size bytes in the FPGA, so
        // we need to calculate appropriately.
        const size_t packet_size_rounded = _round_pkt_size(buff->packet_size());
        send_link->release_send_buff(std::move(buff));

        _fc_state.data_sent(packet_size_rounded);

        if (_fc_state.get_fc_resync_req_pending()
            && _fc_state.dest_has_space(chdr::strc_payload::MAX_PACKET_SIZE)) {
            const auto& xfer_counts = _fc_state.get_xfer_counts();
            const size_t strc_size =
                _round_pkt_size(_fc_sender.send_strc_resync(send_link, xfer_counts));
            _fc_state.clear_fc_resync_req_pending();
            _fc_state.data_sent(strc_size);
        }
    }

    inline size_t _round_pkt_size(const size_t pkt_size_bytes)
    {
        return ((pkt_size_bytes + _chdr_w_bytes - 1) / _chdr_w_bytes) * _chdr_w_bytes;
    }

    /*!
     * Flow control callback for I/O service
     *
     * The I/O service invokes this callback in the send_io::wait_for_dest_ready
     * method.
     *
     * \param num_bytes The number of bytes in the packet to be sent
     * \return Whether there are enough flow control credits for num_bytes
     */
    bool _fc_callback(const size_t num_bytes)
    {
        // No need to round num_bytes since the transport always checks for
        // enough space for a full frame.
        return _fc_state.dest_has_space(num_bytes);
    }

    // Interface to the I/O service
    transport::send_io_if::sptr _send_io;

    // Flow control state
    tx_flow_ctrl_state _fc_state;

    // MTU in bytes
    size_t _mtu = 0;

    // Size of CHDR headers
    size_t _hdr_len = 0;

    // Sequence number for data packets
    uint16_t _data_seq_num = 0;

    // Header to write into send packets
    chdr::chdr_header _send_header;

    // Packet for send data
    chdr::chdr_packet_writer::uptr _send_packet;

    // Packet to receive strs messages
    chdr::chdr_packet_writer::uptr _recv_packet;

    // Handles sending of strc flow control ack packets
    detail::tx_flow_ctrl_sender _fc_sender;

    // Function to enqueue an async msg
    enqueue_async_msg_fn_t _enqueue_async_msg;

    // Local / Source EPID
    sep_id_t _epid;

    //! The CHDR width in bytes.
    size_t _chdr_w_bytes;

    //! The size of the send frame
    size_t _frame_size;

    // Disconnect callback
    disconnect_callback_t _disconnect;
};

}} // namespace uhd::rfnoc
