//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/io_service.hpp>
#include <unordered_map>
#include <list>

namespace uhd { namespace transport {

class inline_recv_mux;
class inline_recv_cb;

/*!
 * Single-threaded I/O service
 * Note this is not an appropriate io_service to use with polling-mode drivers,
 * since such drivers require a thread to poll them and not block (i.e.
 * timeouts are not allowed at the link interface)
 */
class inline_io_service : public virtual io_service,
                          public std::enable_shared_from_this<inline_io_service>
{
public:
    using sptr = std::shared_ptr<inline_io_service>;
    static sptr make(void)
    {
        return sptr(new inline_io_service());
    }

    virtual ~inline_io_service();

    void attach_recv_link(recv_link_if::sptr link) override;
    void attach_send_link(send_link_if::sptr link) override;

    void detach_recv_link(recv_link_if::sptr link) override;
    void detach_send_link(send_link_if::sptr link) override;

    recv_io_if::sptr make_recv_client(recv_link_if::sptr data_link,
        size_t num_recv_frames,
        recv_callback_t cb,
        send_link_if::sptr fc_link,
        size_t num_send_frames,
        recv_io_if::fc_callback_t fc_cb) override;

    send_io_if::sptr make_send_client(send_link_if::sptr send_link,
        size_t num_send_frames,
        send_io_if::send_callback_t send_cb,
        recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        send_io_if::fc_callback_t fc_cb) override;

private:
    friend class inline_recv_io;
    friend class inline_send_io;

    inline_io_service()                         = default;
    inline_io_service(const inline_io_service&) = delete;

    /*!
     * Senders are free to mux a send_link, but the total reserved send_frames
     * must be less than or equal to the link's capacity
     *
     * \param link the link used for sending data
     * \param num_frames number of frames to reserve for this connection
     */
    void connect_sender(send_link_if* link, size_t num_frames);

    /*!
     * Disconnect the sender and free resources
     *
     * \param link the link that was used for sending data
     */
    void disconnect_sender(send_link_if* link);

    /*!
     * Connect a receiver to the link and reserve resources
     * \param link the recv link to use for getting data
     * \param cb a callback for processing received data
     * \param num_frames the number of frames to reserve for this receiver
     */
    void connect_receiver(recv_link_if* link, inline_recv_cb* cb, size_t num_frames);

    /*!
     * Disconnect the receiver from the provided link and free resources
     * \param link the recv link that was used for reception
     * \param cb the callback to disassociate
     */
    void disconnect_receiver(recv_link_if* link, inline_recv_cb* cb);

    /*
     * Function to perform recv operations on a link, which is potentially
     * muxed. Packets are forwarded to the appropriate mux or callback.
     *
     * \param recv_io_cb the callback+interface initiating the operation
     * \param recv_link link to perform receive on
     * \param timeout_ms timeout to wait for a buffer on the link
     * \return a frame_buff uptr with either a buffer with data or no buffer
     */
    frame_buff::uptr recv(
        inline_recv_cb* recv_io_cb, recv_link_if* recv_link, int32_t timeout_ms);

    /*
     * Function to perform recv operations on a link, which is potentially
     * muxed. This function is only called from send_io::release_send_buff, and
     * always expects recv_io_cb to release its incoming buffer. Packets are
     * forwarded to the appropriate mux or callback.
     *
     * \param recv_io_cb the callback+interface initiating the operation
     * \param recv_link link to perform receive on
     * \param timeout_ms timeout to wait for a buffer on the link
     * \return Whether a flow control update was received
     */
    bool recv_flow_ctrl(
        inline_recv_cb* recv_io_cb, recv_link_if* recv_link, int32_t timeout_ms);

    /* Track whether link is muxed and the callback */
    std::unordered_map<recv_link_if*, std::tuple<inline_recv_mux*, inline_recv_cb*>>
        _recv_tbl;

    /* Shared ptr kept to avoid untimely release */
    std::list<send_link_if::sptr> _send_links;
    std::list<recv_link_if::sptr> _recv_links;
};

}} // namespace uhd::transport
