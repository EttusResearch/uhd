//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhdlib/rfnoc/async_msg_handler.hpp>
#include <boost/make_shared.hpp>
#include <mutex>

using namespace uhd;
using namespace uhd::rfnoc;

template <endianness_t _endianness>
class async_msg_handler_impl : public async_msg_handler
{
public:
    /************************************************************************
     * Types
     ***********************************************************************/
    typedef uhd::transport::bounded_buffer<async_msg_t> async_md_type;

    /************************************************************************
     * Structors
     ***********************************************************************/
    async_msg_handler_impl(uhd::transport::zero_copy_if::sptr recv,
        uhd::transport::zero_copy_if::sptr send,
        uhd::sid_t sid)
        : _rx_xport(recv), _tx_xport(send), _sid(sid)
    {
        // Launch receive thread
        _recv_msg_task = task::make([=]() { this->handle_async_msgs(); });
    }

    ~async_msg_handler_impl() {}

    /************************************************************************
     * API calls
     ***********************************************************************/
    int register_event_handler(
        const async_msg_t::event_code_t event_code, async_handler_type handler)
    {
        _event_handlers.insert(std::pair<async_msg_t::event_code_t, async_handler_type>(
            event_code, handler));
        return _event_handlers.count(event_code);
    }

    void post_async_msg(const async_msg_t& metadata)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto const event_handler : _event_handlers) {
            // If the event code in the message matches the event code used at
            // registration time, call the event handler
            if ((metadata.event_code & event_handler.first) == event_handler.first) {
                event_handler.second(metadata);
            }
        }

        // Print
        if (metadata.event_code & async_msg_t::EVENT_CODE_UNDERFLOW) {
            UHD_LOG_FASTPATH("U")
        } else if (metadata.event_code
                   & (async_msg_t::EVENT_CODE_SEQ_ERROR
                         | async_msg_t::EVENT_CODE_SEQ_ERROR_IN_BURST)) {
            UHD_LOG_FASTPATH("S")
        } else if (metadata.event_code
                   & (async_msg_t::EVENT_CODE_LATE_CMD_ERROR
                         | async_msg_t::EVENT_CODE_LATE_DATA_ERROR)) {
            UHD_LOG_FASTPATH("L")
        } else if (metadata.event_code & async_msg_t::EVENT_CODE_OVERRUN) {
            UHD_LOG_FASTPATH("O")
        }
    }

private: // methods
    /************************************************************************
     * Internals
     ***********************************************************************/
    /*! Packet receiver thread call.
     */
    void handle_async_msgs()
    {
        using namespace uhd::transport;
        managed_recv_buffer::sptr buff = _rx_xport->get_recv_buff();
        if (not buff)
            return;

        // Get packet info
        vrt::if_packet_info_t if_packet_info;
        if_packet_info.num_packet_words32 = buff->size() / sizeof(uint32_t);
        const uint32_t* packet_buff       = buff->cast<const uint32_t*>();

        // unpacking can fail
        uint32_t (*endian_conv)(uint32_t) = uhd::ntohx;
        try {
            if (_endianness == ENDIANNESS_BIG) {
                vrt::chdr::if_hdr_unpack_be(packet_buff, if_packet_info);
                endian_conv = uhd::ntohx;
            } else {
                vrt::chdr::if_hdr_unpack_le(packet_buff, if_packet_info);
                endian_conv = uhd::wtohx;
            }
        } catch (const uhd::value_error& ex) {
            UHD_LOGGER_ERROR("RFNOC")
                << "[async message handler] Error parsing async message packet: "
                << ex.what() << std::endl;
            return;
        }

        // We discard anything that's not actually a command or response packet.
        if (not(if_packet_info.packet_type & vrt::if_packet_info_t::PACKET_TYPE_CMD)
            or if_packet_info.num_packet_words32 == 0) {
            return;
        }

        const uint32_t* payload = packet_buff + if_packet_info.num_header_words32;
        async_msg_t metadata(if_packet_info.num_payload_words32 - 1);
        metadata.has_time_spec = if_packet_info.has_tsf;
        // FIXME: not hardcoding tick rate
        metadata.time_spec  = time_spec_t::from_ticks(if_packet_info.tsf, 1);
        metadata.event_code = async_msg_t::event_code_t(endian_conv(payload[0]) & 0xFFFF);
        metadata.sid        = if_packet_info.sid;

        // load user payload
        for (size_t i = 1; i < if_packet_info.num_payload_words32; i++) {
            metadata.payload[i - 1] = endian_conv(payload[i]);
        }

        this->post_async_msg(metadata);
    }

    uint32_t get_local_addr() const
    {
        return _sid.get_src();
    }

private: // members
    std::mutex _mutex;
    //! Store event handlers
    std::multimap<async_msg_t::event_code_t, async_handler_type> _event_handlers;
    //! port that receive messge
    uhd::transport::zero_copy_if::sptr _rx_xport;

    //! port that send out respond
    uhd::transport::zero_copy_if::sptr _tx_xport;

    //! The source part of \p _sid is the address of this async message handler.
    uhd::sid_t _sid;

    //! Stores the task that polls the Rx queue
    task::sptr _recv_msg_task;
};

async_msg_handler::sptr async_msg_handler::make(uhd::transport::zero_copy_if::sptr recv,
    uhd::transport::zero_copy_if::sptr send,
    uhd::sid_t sid,
    endianness_t endianness)
{
    if (endianness == ENDIANNESS_BIG) {
        return boost::make_shared<async_msg_handler_impl<ENDIANNESS_BIG>>(
            recv, send, sid);
    } else {
        return boost::make_shared<async_msg_handler_impl<ENDIANNESS_LITTLE>>(
            recv, send, sid);
    }
}
