//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DEVICE3_FLOW_CTRL_HPP
#define INCLUDED_DEVICE3_FLOW_CTRL_HPP

#include "device3_impl.hpp"
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/utils/log.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd { namespace usrp {

//! Stores the state of RX flow control
struct rx_fc_cache_t
{
    rx_fc_cache_t()
        : interval(0)
        , last_byte_count(0)
        , total_bytes_consumed(0)
        , total_packets_consumed(0)
        , seq_num(0)
    {
    }

    //! Flow control interval in bytes
    size_t interval;
    //! Byte count at last flow control packet
    uint32_t last_byte_count;
    //! This will wrap around, but that's OK, because math.
    uint32_t total_bytes_consumed;
    //! This will wrap around, but that's OK, because math.
    uint32_t total_packets_consumed;
    //! Sequence number of next flow control packet
    uint64_t seq_num;
    uhd::sid_t sid;
    uhd::transport::zero_copy_if::sptr xport;
    std::function<uint32_t(uint32_t)> to_host;
    std::function<uint32_t(uint32_t)> from_host;
    std::function<void(
        const uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        unpack;
    std::function<void(uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        pack;
};

/*! Send out RX flow control packets.
 *
 * This function handles updating the counters for the consumed
 * bytes and packets, determines if a flow control message is
 * is necessary, and sends one if it is.  Passing a nullptr for
 * the buff parameter will skip the counter update.
 *
 * \param fc_cache RX flow control state information
 * \param buff Receive buffer.  Setting to nullptr will
 *             skip the counter update.
 */
inline bool rx_flow_ctrl(
    boost::shared_ptr<rx_fc_cache_t> fc_cache, uhd::transport::managed_buffer::sptr buff)
{
    // If the caller supplied a buffer
    if (buff) {
        // Unpack the header
        uhd::transport::vrt::if_packet_info_t packet_info;
        packet_info.num_packet_words32 = buff->size() / sizeof(uint32_t);
        const uint32_t* pkt            = buff->cast<const uint32_t*>();
        try {
            fc_cache->unpack(pkt, packet_info);
        } catch (const std::exception& ex) {
            // Log and ignore
            UHD_LOGGER_ERROR("RX FLOW CTRL")
                << "Error unpacking packet: " << ex.what() << std::endl;
            return true;
        }

        // Update counters assuming the buffer is a consumed packet
        if (not packet_info.error) {
            const size_t bytes = 4 * (packet_info.num_header_words32 + packet_info.num_payload_words32);
            fc_cache->total_bytes_consumed += bytes;
            fc_cache->total_packets_consumed++;
        }
    }

    // Just return if there is no need to send a flow control packet
    if (fc_cache->total_bytes_consumed - fc_cache->last_byte_count < fc_cache->interval) {
        return true;
    }

    // Time to send a flow control packet
    // Get a send buffer
    uhd::transport::managed_send_buffer::sptr fc_buff =
        fc_cache->xport->get_send_buff(0.0);
    if (not fc_buff) {
        throw uhd::runtime_error("rx_flowctrl timed out getting a send buffer");
    }
    uint32_t* pkt = fc_buff->cast<uint32_t*>();

    // load packet info
    uhd::transport::vrt::if_packet_info_t packet_info;
    packet_info.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_FC;
    packet_info.num_payload_words32 = uhd::usrp::DEVICE3_FC_PACKET_LEN_IN_WORDS32;
    packet_info.num_payload_bytes   = packet_info.num_payload_words32 * sizeof(uint32_t);
    packet_info.packet_count        = fc_cache->seq_num++;
    packet_info.sob                 = false;
    packet_info.eob                 = false;
    packet_info.error               = false;
    packet_info.fc_ack              = false;
    packet_info.sid                 = fc_cache->sid.get();
    packet_info.has_sid             = true;
    packet_info.has_cid             = false;
    packet_info.has_tsi             = false;
    packet_info.has_tsf             = false;
    packet_info.has_tlr             = false;

    // Load Header:
    fc_cache->pack(pkt, packet_info);
    // Load Payload: Packet count, and byte count
    pkt[packet_info.num_header_words32 + uhd::usrp::DEVICE3_FC_PACKET_COUNT_OFFSET] =
        fc_cache->from_host(fc_cache->total_packets_consumed);
    pkt[packet_info.num_header_words32 + uhd::usrp::DEVICE3_FC_BYTE_COUNT_OFFSET] =
        fc_cache->from_host(fc_cache->total_bytes_consumed);

    // send the buffer over the interface
    fc_buff->commit(sizeof(uint32_t) * (packet_info.num_packet_words32));

    // update byte count
    fc_cache->last_byte_count = fc_cache->total_bytes_consumed;

    return true;
}

/*! Handle RX flow control ACK packets.
 *
 */
inline void handle_rx_flowctrl_ack(
    boost::shared_ptr<rx_fc_cache_t> fc_cache, const uint32_t* payload)
{
    const uint32_t pkt_count  = fc_cache->to_host(payload[0]);
    const uint32_t byte_count = fc_cache->to_host(payload[1]);
    if (fc_cache->total_bytes_consumed != byte_count) {
        UHD_LOGGER_DEBUG("device3")
            << "oh noes: byte_count==" << byte_count
            << "  total_bytes_consumed==" << fc_cache->total_bytes_consumed << std::hex
            << " sid==" << fc_cache->sid << std::dec << std::endl;
    }
    fc_cache->total_bytes_consumed   = byte_count;
    fc_cache->total_packets_consumed = pkt_count; // guess we need a pkt offset too?

    // This will send a flow control packet if there is a significant discrepancy
    rx_flow_ctrl(fc_cache, nullptr);
}

//! Stores the state of TX flow control
struct tx_fc_cache_t
{
    tx_fc_cache_t(uint32_t capacity)
        : last_byte_ack(0)
        , last_seq_ack(0)
        , byte_count(0)
        , pkt_count(0)
        , window_size(capacity)
        , fc_ack_seqnum(0)
        , fc_received(false)
    {
    }

    uint32_t last_byte_ack;
    uint32_t last_seq_ack;
    uint32_t byte_count;
    uint32_t pkt_count;
    uint32_t window_size;
    uint32_t fc_ack_seqnum;
    bool fc_received;
    std::function<uint32_t(uint32_t)> to_host;
    std::function<uint32_t(uint32_t)> from_host;
    std::function<void(
        const uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        unpack;
    std::function<void(uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        pack;
};

inline bool tx_flow_ctrl(boost::shared_ptr<tx_fc_cache_t> fc_cache,
    uhd::transport::zero_copy_if::sptr xport,
    uhd::transport::managed_buffer::sptr buff)
{
    while (true) {
        // If there is space
        if (fc_cache->window_size - (fc_cache->byte_count - fc_cache->last_byte_ack)
            >= buff->size()) {
            // All is good - packet will be sent
            fc_cache->byte_count += buff->size();
            // Round up to nearest word
            if (fc_cache->byte_count % uhd::usrp::DEVICE3_LINE_SIZE) {
                fc_cache->byte_count +=
                    uhd::usrp::DEVICE3_LINE_SIZE
                    - (fc_cache->byte_count % uhd::usrp::DEVICE3_LINE_SIZE);
            }
            fc_cache->pkt_count++;
            return true;
        }

        // Look for a flow control message to update the space available in the buffer.
        uhd::transport::managed_recv_buffer::sptr buff = xport->get_recv_buff(0.1);
        if (buff) {
            uhd::transport::vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size() / sizeof(uint32_t);
            const uint32_t* packet_buff       = buff->cast<const uint32_t*>();
            try {
                fc_cache->unpack(packet_buff, if_packet_info);
            } catch (const std::exception& ex) {
                UHD_LOGGER_ERROR("TX FLOW CTRL")
                    << "Error unpacking flow control packet: " << ex.what() << std::endl;
                continue;
            }

            if (if_packet_info.packet_type
                != uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_FC) {
                UHD_LOGGER_ERROR("TX FLOW CTRL")
                    << "Unexpected packet received by flow control handler: "
                    << if_packet_info.packet_type << std::endl;
                continue;
            }

            const uint32_t* payload   = &packet_buff[if_packet_info.num_header_words32];
            const uint32_t pkt_count  = fc_cache->to_host(payload[0]);
            const uint32_t byte_count = fc_cache->to_host(payload[1]);

            // update the amount of space
            fc_cache->last_byte_ack = byte_count;
            fc_cache->last_seq_ack  = pkt_count;

            fc_cache->fc_received = true;
        }
    }
    return false;
}

inline void tx_flow_ctrl_ack(boost::shared_ptr<tx_fc_cache_t> fc_cache,
    uhd::transport::zero_copy_if::sptr send_xport,
    uhd::sid_t send_sid)
{
    if (not fc_cache->fc_received) {
        return;
    }

    // Time to send a flow control ACK packet
    // Get a send buffer
    uhd::transport::managed_send_buffer::sptr fc_buff = send_xport->get_send_buff(0.0);
    if (not fc_buff) {
        UHD_LOGGER_ERROR("tx_flow_ctrl_ack") << "timed out getting a send buffer";
        return;
    }
    uint32_t* pkt = fc_buff->cast<uint32_t*>();

    // Load packet info
    uhd::transport::vrt::if_packet_info_t packet_info;
    packet_info.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_ACK;
    packet_info.num_payload_words32 = uhd::usrp::DEVICE3_FC_PACKET_LEN_IN_WORDS32;
    packet_info.num_payload_bytes   = packet_info.num_payload_words32 * sizeof(uint32_t);
    packet_info.packet_count        = fc_cache->fc_ack_seqnum++;
    packet_info.sob                 = false;
    packet_info.eob                 = true;
    packet_info.error               = false;
    packet_info.fc_ack              = false;
    packet_info.sid                 = send_sid.get();
    packet_info.has_sid             = true;
    packet_info.has_cid             = false;
    packet_info.has_tsi             = false;
    packet_info.has_tsf             = false;
    packet_info.has_tlr             = false;

    // Load Header:
    fc_cache->pack(pkt, packet_info);

    // Update counters to include this packet
    size_t fc_ack_pkt_size = sizeof(uint32_t) * (packet_info.num_packet_words32);
    fc_cache->byte_count += fc_ack_pkt_size;
    // Round up to nearest word
    if (fc_cache->byte_count % uhd::usrp::DEVICE3_LINE_SIZE) {
        fc_cache->byte_count += uhd::usrp::DEVICE3_LINE_SIZE
                                - (fc_cache->byte_count % uhd::usrp::DEVICE3_LINE_SIZE);
    }
    fc_cache->pkt_count++;

    // Load Payload: Packet count, and byte count
    pkt[packet_info.num_header_words32 + uhd::usrp::DEVICE3_FC_PACKET_COUNT_OFFSET] =
        fc_cache->from_host(fc_cache->pkt_count);
    pkt[packet_info.num_header_words32 + uhd::usrp::DEVICE3_FC_BYTE_COUNT_OFFSET] =
        fc_cache->from_host(fc_cache->byte_count);

    // Send the buffer over the interface
    fc_buff->commit(fc_ack_pkt_size);

    // Reset for next FC
    fc_cache->fc_received = false;
}

}}; // namespace uhd::usrp

#endif /* INCLUDED_DEVICE3_FLOW_CTRL_HPP */
