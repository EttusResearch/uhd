//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/adapter_id.hpp>
#include <uhd/transport/frame_buff.hpp>
#include <memory>

namespace uhd { namespace transport {

/*!
 * Link interface for transmitting packets.
 */
class send_link_if
{
public:
    using sptr = std::shared_ptr<send_link_if>;

    /*!
     * Get the number of frame buffers that can be queued by this link.
     */
    virtual size_t get_num_send_frames() const = 0;

    /*!
     * Get the maximum capacity of a frame buffer.
     */
    virtual size_t get_send_frame_size() const = 0;

    /*!
     * Get an empty frame buffer in which to write packet contents.
     *
     * \param timeout_ms a positive timeout value specifies the maximum number
                         of ms to wait, a negative value specifies to block
                         until successful, and a value of 0 specifies no wait.
     * \return a frame buffer, or null uptr if timeout occurs
     */
    virtual frame_buff::uptr get_send_buff(int32_t timeout_ms) = 0;

    /*!
     * Send a packet with the contents of the frame buffer and release the
     * buffer, allowing the link driver to reuse it. If the size of the frame
     * buffer is 0, the buffer is released with no packet being sent.
     *
     * \param buffer frame buffer containing packet data
     *
     * Throws an exception if an I/O error occurs while sending
     */
    virtual void release_send_buff(frame_buff::uptr buff) = 0;

    /*!
     * Get the physical adapter id used for this link
     */
    virtual adapter_id_t get_send_adapter_id() const = 0;

    /*!
     * Returns whether this link type supports releasing the frame buffers
     * in an order different from that in which they were acquired.
     */
    virtual bool supports_send_buff_out_of_order() const
    {
        return true;
    }

    send_link_if()                               = default;
    send_link_if(const send_link_if&)            = delete;
    send_link_if& operator=(const send_link_if&) = delete;
};

/*!
 * Link interface for receiving packets.
 */
class recv_link_if
{
public:
    using sptr = std::shared_ptr<recv_link_if>;

    /*!
     * Get the number of frame buffers that can be queued by this link.
     */
    virtual size_t get_num_recv_frames() const = 0;

    /*!
     * Get the maximum capacity of a frame buffer.
     */
    virtual size_t get_recv_frame_size() const = 0;

    /*!
     * Receive a packet and return a frame buffer containing the packet data.
     *
     * \param timeout_ms a positive timeout value specifies the maximum number
                         of ms to wait, a negative value specifies to block
                         until successful, and a value of 0 specifies no wait.
     * \return a frame buffer, or null uptr if timeout occurs
     */
    virtual frame_buff::uptr get_recv_buff(int32_t timeout_ms) = 0;

    /*!
     * Release a frame buffer, allowing the link driver to reuse it.
     *
     * \param buffer frame buffer to release for reuse by the link
     */
    virtual void release_recv_buff(frame_buff::uptr buff) = 0;

    /*!
     * Get the physical adapter ID used for this link
     */
    virtual adapter_id_t get_recv_adapter_id() const = 0;

    /*!
     * Returns whether this link type supports releasing the frame buffers
     * in an order different from that in which they were acquired.
     */
    virtual bool supports_recv_buff_out_of_order() const
    {
        return true;
    }

    recv_link_if()                               = default;
    recv_link_if(const recv_link_if&)            = delete;
    recv_link_if& operator=(const recv_link_if&) = delete;
};

}} // namespace uhd::transport
