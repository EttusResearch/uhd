//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
//
/*!
 * \file
 * This file declares the I/O service interfaces. There are two layers to the
 * implementation of a transport:
 *
 *  - The link layer is the lower of the two layers. Link layer interfaces
 *    (uhd::transport::recv_link_if and uhd::transport::send_link_if) provide
 *    an abstraction for transmission of frames over a physical layer.
 *  - Transports are a layer above the link layer and provide flow control,
 *    multiplexing, and the ability to offload I/O to worker threads.
 *  - Transports are implemented using I/O services
 *    (uhd::transport::io_service). Transports implement callbacks defined by
 *    the I/O service, and the I/O service implements the scheduling of the work
 *    defined by the callbacks. Different I/O service implementations exist to
 *    implement different work scheduling policies.
 *
 * Callback parameters include pointers to the send and recv links. In a
 * multithreaded I/O service, the methods defined by the transport run in the
 * caller thread, while the callbacks run in a worker thread. Transport
 * implementations must be careful to ensure any shared data between the
 * transport methods and the callbacks it implements is thread-safe. Callbacks
 * should never sleep since doing so would stall worker threads that may be
 * servicing multiple transports.
 *
 * Only the callbacks should interact directly with the links. The public
 * transport methods request and release buffers from the I/O service using the
 * uhd::transport::recv_io_if and uhd::transport::send_io_if interfaces.
 */


#pragma once

#include <uhdlib/transport/link_if.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace transport {

/*!
 * Callback that a transport must implement to process received packets.
 * Function should make a determination of whether the packet belongs to it
 * and return the bool.
 *
 * Function may consume and release the buffer internally (if packet was
 * destined for it). The recv_link_if may be used to release it, and the
 * provided frame_buff::uptr must be made to relinquish ownership before the
 * callback returns. If the buffer was not destined for the callback's
 * transport, the buffer must NOT be released, and the uptr must remain intact.
 *
 * For uhd::transport::recv_io_if clients:
 *   - A buffer that is not released (but destined for this client) will be
 *     queued up to be returned on a uhd::transport::recv_io_if::get_recv_buff()
 *     call.
 *   - The send_link_if may be used to send automatic responses, such as a
 *     simple response to a query of flow control state.
 *
 * For uhd::transport::send_io_if clients:
 *   - The buffer must either be destined for this client and released, or it
 *     must not be for this client and left unreleased.
 *   - Currently, the send_link_if is always null and should not be used.
 *
 * Callbacks execute on the I/O thread! Callback implementations must take care
 * with what state is touched. In addition, this callback should NOT sleep.
 *
 * \param frame_buff the buffer that was received
 * \param recv_link_if the link used to retrieve the buffer. Can be used to
 *  release the buffer back to the link, if buffer is consumed internally.
 * \param send_link_if a link for sending a response. Can be null.
 * \return true if buffer matched this transport, false otherwise
 */
using recv_callback_t =
    std::function<bool(frame_buff::uptr&, recv_link_if*, send_link_if*)>;

/*!
 * Callback to disconnect links.  This allows a function to be registered
 * that can call back to the io_service_mgr to completely disconnect the
 * links.
 */
using disconnect_callback_t = std::function<void()>;

/*!
 * Interface for a recv transport to request/release buffers from a link. A
 * recv transport is a transport with a primary purpose of receiving data, and
 * the recv_io_if class interacts with the io_service to schedule the data
 * movement, including any queuing when there are worker threads.
 *
 * recv_io_if::get_recv_buff and recv_io_if::release_recv_buff are the pathways
 * for data reception, and the uhd::transport::recv_callback_t and
 * recv_io_if::fc_callback_t are available for handling flow control and
 * automatic responses. The uhd::transport::recv_callback_t also indicates
 * whether the buffer is either
 *   - to go up to the transport above,
 *   - destined to this transport but consumed by it,
 *   - or not destined for this transport and left alone.
 */
class recv_io_if
{
public:
    using sptr = std::shared_ptr<recv_io_if>;

    virtual ~recv_io_if() = default;

    /*!
     * Callback for producing a flow control response (or any other response
     * needed when a received frame_buff is released via
     * recv_io_if::release_recv_buff()).
     *
     * The callback must release the buffer, but it can update internal state
     * as well. It can also send a response with the send_link_if, should it
     * desire to do so.
     *
     * Callbacks execute on the I/O thread: Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     */
    using fc_callback_t =
        std::function<void(frame_buff::uptr, recv_link_if*, send_link_if*)>;

    /* Transport client methods */
    /*!
     * Gets a receive buffer from the I/O service.
     *
     * Multi-thread version:
     *   - Check queue, and wait on queue.
     *
     * Single-thread version:
     *   - Do receive on the link, and wait on link.
     *
     * \param timeout_ms The timeout in milliseconds
     * \return if timeout, an empty frame_buff::uptr, else a buffer with data
     */
    virtual frame_buff::uptr get_recv_buff(int32_t timeout_ms) = 0;

    /*!
     * Release buffer back to the I/O service.
     *
     * \param buff the buffer to release
     */
    virtual void release_recv_buff(frame_buff::uptr buff) = 0;

    /*!
     * Get number of send frames reserved by this I/O interface.
     *
     * \return Number of frames reserved
     */
    size_t get_num_send_frames(void) const
    {
        return _num_send_frames;
    }

    /*!
     * Get number of recv frames reserved by this I/O interface.
     *
     * \return Number of frames reserved
     */
    size_t get_num_recv_frames(void) const
    {
        return _num_recv_frames;
    }

protected:
    /*! Number of frames reserved on the send_link_if associated with this */
    size_t _num_send_frames;

    /*! Number of frames reserved on the recv_link_if associated with this */
    size_t _num_recv_frames;
};

/*!
 * Interface for a send transport to request/release buffers from a link. A
 * send transport is a transport with a primary purpose of sending data, and
 * the send_io_if class interacts with the io_service to schedule the data
 * movement, including any queuing when there are worker threads.
 *
 * send_io_if::get_send_buff and send_io_if::release_send_buff are the pathways
 * for data transmission, and the uhd::transport::recv_callback_t and
 * send_io_if::send_callback_t are available for handling flow control and
 * receiving side-band messages. Currently, the uhd::transport::recv_callback_t
 * would be provided only a NULL send_link_if. Thus, when a buffer is destined
 * for this transport, it must consume the buffer and release it back to the
 * recv_link_if.
 */
class send_io_if
{
public:
    using sptr = std::shared_ptr<send_io_if>;

    virtual ~send_io_if() = default;

    /*!
     * Callback for sending the packet. Callback should call release_send_buff()
     * and update any internal state needed. For example, flow control state
     * could be updated here, and the header could be filled out as well, like
     * the packet's sequence number and/or addresses.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     */
    using send_callback_t = std::function<void(frame_buff::uptr, send_link_if*)>;

    /*!
     * Callback to check whether a packet can be sent. For flow controlled
     * links, the callback should return whether the requested number of bytes
     * can be received by the destination.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     */
    using fc_callback_t = std::function<bool(const size_t)>;

    /* Transport client methods */
    /*!
     * Get an empty send buffer from the link.
     *
     * \param timeout_ms timeout in milliseconds to wait for a buffer
     * \return If no buffer available, return an empty frame_buff:uptr. Else
     *         return an empty frame_buff with its packet_size set to the
     *         maximum capacity of the buffer.
     */
    virtual frame_buff::uptr get_send_buff(int32_t timeout_ms) = 0;

    /*!
     * Wait until the destination is ready for a packet. For flow controlled
     * transports, this method must be called prior to release_send_buff. If
     * the transport is not flow controlled, you do not need to call this
     * method.
     *
     * \param num_bytes the number of bytes to be sent in release_send_buff
     * \param timeout_ms timeout in milliseconds to wait for destination to be
     *                   ready
     * \return whether the destination is ready for the requested bytes
     */
    virtual bool wait_for_dest_ready(size_t num_bytes, int32_t timeout_ms) = 0;

    /*!
     * Release the send buffer to the send queue.
     * If the frame_buff's packet_size is zero, the link will free the buffer
     * without sending it.
     *
     * \param buff the buffer to be freed/sent
     */
    virtual void release_send_buff(frame_buff::uptr buff) = 0;

    /*!
     * Get number of send frames reserved by this I/O interface.
     *
     * \return Number of frames reserved
     */
    size_t get_num_send_frames(void) const
    {
        return _num_send_frames;
    }

    /*!
     * Get number of recv frames reserved by this I/O interface.
     *
     * \return Number of frames reserved
     */
    size_t get_num_recv_frames(void) const
    {
        return _num_recv_frames;
    }

protected:
    /*! Number of frames reserved on the send_link_if associated with this */
    size_t _num_send_frames;

    /*! Number of frames reserved on the recv_link_if associated with this */
    size_t _num_recv_frames;
};

/*!
 * Together with the recv_io_if and send_io_if, connects transports to links
 * and schedules I/O.
 *
 * Transports gain access to the links via io_service::make_send_client() and
 * io_service::make_recv_client(). These functions produce the conduits to
 * send and recv data through the links (via send_io_if and recv_io_if,
 * respectively). Note that muxing of send_link_if and recv_link_if is allowed,
 * but there may be a balance needed for performance / ease-of-use / resource
 * utilization.
 *
 * In all implementations, only one thread may do the work of a single
 * io_service object. If the I/O service does not provide worker threads, its
 * client interfaces and send_io_if / recv_io_if implementations cannot be
 * shared by multiple threads. Thus, without worker threads, a muxed link must
 * have all clients on the same thread (or a synchronization function
 * protecting access).
 */
class io_service
{
public:
    using sptr = std::shared_ptr<io_service>;

    /*!
     * Attach a recv_link_if to be serviced by this I/O service.
     *
     * \param link the recv_link_if to attach
     */
    virtual void attach_recv_link(recv_link_if::sptr link) = 0;

    /*!
     * Attach a send_link_if to be serviced by this I/O service.
     *
     * \param link the send_link_if to attach
     */
    virtual void attach_send_link(send_link_if::sptr link) = 0;

    /*!
     * Detach a recv_link_if previously attached to this I/O service.
     *
     * \param link the recv_link_if to detach
     */
    virtual void detach_recv_link(recv_link_if::sptr link) = 0;

    /*!
     * Detach a send_link_if previously attached to this I/O service.
     *
     * \param link the send_link_if to detach
     */
    virtual void detach_send_link(send_link_if::sptr link) = 0;

    /*!
     * Create a send_io_if so a transport may send packets through the link.
     *
     * Throws exception if cannot reserve requested frames.
     *
     * \param send_link the link to use for sending data
     * \param num_send_frames Number of buffers to reserve in data_link
     * \param send_cb callback function for buffer processing just before sending
     * \param recv_link the link used to observe flow control (can be empty)
     * \param num_recv_frames Number of buffers to reserve in recv_link
     * \param recv_cb callback function for receiving packets from recv_link
     * \param fc_cb callback function to check if destination is ready for data
     * \return a send_io_if for interfacing with the link
     */
    virtual send_io_if::sptr make_send_client(send_link_if::sptr send_link,
        size_t num_send_frames,
        send_io_if::send_callback_t cb,
        recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        send_io_if::fc_callback_t fc_cb) = 0;

    /*!
     * Create a recv_io_if and registers the transport's callbacks.
     *
     * Throws exception if cannot reserve requested frames.
     *
     * \param data_link the link to receive packets from
     * \param cb callback function for processing received packets
     * \param num_recv_frames Number of buffers to reserve in data_link
     * \param fc_link the link used to send flow control responses
     * \param fc_cb callback function for handling flow control
     * \param num_send_frames Number of buffers to reserve in fc_link
     * \return a recv_io_if for interfacing with the link
     */
    virtual recv_io_if::sptr make_recv_client(recv_link_if::sptr data_link,
        size_t num_recv_frames,
        recv_callback_t cb,
        send_link_if::sptr fc_link,
        size_t num_send_frames,
        recv_io_if::fc_callback_t fc_cb) = 0;

    io_service()                             = default;
    io_service(const io_service&)            = delete;
    io_service& operator=(const io_service&) = delete;
    virtual ~io_service()                    = default;
};

}} // namespace uhd::transport
