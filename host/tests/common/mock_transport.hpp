//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHDLIB_TRANSPORT_TRANSPORT_IF_HPP
#define INCLUDED_UHDLIB_TRANSPORT_TRANSPORT_IF_HPP

#include <uhd/exception.hpp>
#include <uhdlib/transport/io_service.hpp>
// Must come before the following include to fix an issue with Boost 1.67
#include <boost/next_prior.hpp>
///
#include <boost/lockfree/spsc_queue.hpp>
#include <utility>

namespace uhd { namespace transport {

namespace {
constexpr size_t ADDR_OFFSET  = 0;
constexpr size_t TYPE_OFFSET  = 1; /* 0 for data, 1 for FC, 2 for msg */
constexpr size_t SEQNO_OFFSET = 2; /* For FC, this is last seen seqno */
constexpr size_t LEN_OFFSET   = 3;
constexpr size_t DATA_OFFSET  = 4;
constexpr size_t MSG_BUFFS    = 8;
}; // namespace
/*!
 * Mock transport with following packet format:
 * Data: [dst_addr/src_addr, type, seqno, data_len, data...]
 * FC: [dst_addr/src_addr, type, ackno]
 * Msg: [dst_addr/src_addr, type, null, data]
 * All fields are 32-bit words (dst_addr and src_addr are 16 bits each)
 */
class mock_send_transport
{
public:
    using sptr = std::shared_ptr<mock_send_transport>;

    mock_send_transport(io_service::sptr io_srv,
        send_link_if::sptr send_link,
        recv_link_if::sptr recv_link,
        uint16_t dst_addr,
        uint16_t src_addr,
        uint32_t credits)
        : _credits(credits), _frame_size(send_link->get_send_frame_size())
    {
        _send_addr = (dst_addr << 16) | (src_addr << 0);
        _recv_addr = (src_addr << 16) | (dst_addr << 0);

        /* Make message client for sending side-band messages */
        send_io_if::send_callback_t msg_send_cb = [this](frame_buff::uptr buff,
                                                      send_link_if* link) {
            uint32_t* data    = (uint32_t*)buff->data();
            data[ADDR_OFFSET] = this->_send_addr;
            data[TYPE_OFFSET] = 2; /* MSG type */
            link->release_send_buff(std::move(buff));
        };
        _msg_if = io_srv->make_send_client(
            send_link, MSG_BUFFS, msg_send_cb, recv_link_if::sptr(), 0, nullptr, nullptr);

        /* Make client for sending streaming data */
        send_io_if::send_callback_t send_cb = [this](frame_buff::uptr buff,
                                                  send_link_if* link) {
            this->send_buff(std::move(buff), link);
        };
        recv_callback_t recv_cb = [this](frame_buff::uptr& buff,
                                      recv_link_if* link,
                                      send_link_if* /*send_link*/) {
            return this->recv_buff(buff, link);
        };
        send_io_if::fc_callback_t fc_cb = [this](const size_t bytes) {
            return this->can_send(bytes);
        };

        /* Pretend get 1 flow control message per sent packet */
        _send_if = io_srv->make_send_client(
            send_link, credits, send_cb, recv_link, credits, recv_cb, fc_cb);
    }

    ~mock_send_transport() {}

    /*!
     * Get a buffer for creating a non-flow-controlled message
     */
    bool put_msg(uint32_t msg, int32_t timeout_ms)
    {
        frame_buff::uptr buff = _msg_if->get_send_buff(timeout_ms);
        if (!buff) {
            return false;
        }
        uint32_t* data    = (uint32_t*)buff->data();
        data[TYPE_OFFSET] = 2;
        data[DATA_OFFSET] = msg;
        buff->set_packet_size((1 + DATA_OFFSET) * sizeof(uint32_t));
        _msg_if->release_send_buff(std::move(buff));
        return true;
    }

    /*!
      * Get an empty frame buffer in which to write packet contents.
      *
      * \param timeout_ms a positive timeout value specifies the maximum number
                          of ms to wait, a negative value specifies to block
                          until successful, and a value of 0 specifies no wait.
      * \return a frame buffer, or null uptr if timeout occurs
      */
    frame_buff::uptr get_data_buff(int32_t timeout_ms)
    {
        if (!_send_if->wait_for_dest_ready(_frame_size, timeout_ms)) {
            return frame_buff::uptr();
        }

        frame_buff::uptr buff = _send_if->get_send_buff(timeout_ms);
        if (!buff) {
            return frame_buff::uptr();
        }
        uint32_t* data    = (uint32_t*)buff->data();
        data[TYPE_OFFSET] = 0;
        return frame_buff::uptr(std::move(buff));
    }

    /*!
     * Release a frame buffer, allowing the driver to reuse it.
     *
     * \param buffer frame buffer to release for reuse by the link
     */
    void release_data_buff(frame_buff::uptr& buff, size_t len)
    {
        if (len == 0) {
            _send_if->release_send_buff(std::move(buff));
            return;
        }
        uint32_t* data   = (uint32_t*)buff->data();
        data[LEN_OFFSET] = len;
        buff->set_packet_size((len + DATA_OFFSET) * sizeof(uint32_t));
        _send_if->release_send_buff(std::move(buff));
    }

    /*!
     * Callback for sending the packet. Callback is responsible for calling
     * release_send_buff() if it wants to send the packet. This will require
     * moving the uptr's reference. If the packet will NOT be sent, the
     * callback must NOT release the uptr.
     *
     * Function should update any internal state needed. For example, flow
     * control state could be updated here, and the header could be filled out
     * as well, like the packet's sequence number and/or addresses.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     */
    void send_buff(frame_buff::uptr buff, send_link_if* send_link)
    {
        uint32_t* data     = (uint32_t*)buff->data();
        data[ADDR_OFFSET]  = _send_addr;
        data[SEQNO_OFFSET] = _seqno;
        send_link->release_send_buff(std::move(buff));
        _seqno++;
    }

    bool can_send(size_t /* bytes */)
    {
        return _seqno < _ackno + _credits;
    };

    /*!
     * Callback for when packets are received (for processing).
     * Function should make a determination of whether the packet belongs to it
     * and return the bool.
     *
     * Function may consume and release the buffer internally (if packet was
     * destined for it). The recv_link_if may be used to release it, and the
     * provided frame_buff::uptr must relinquish ownership before returning.
     * If the buffer was not destined for the user of this function, buffer must
     * NOT be released, and the uptr must remain intact.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     *
     * \param frame_buff the buffer that was received
     * \param recv_link_if the link used to retrieve the buffer. Can be used to
     *  release the buffer back to the link, if buffer is consumed internally.
     * \return true if buffer matched this transport, false otherwise
     */
    bool recv_buff(frame_buff::uptr& buff, recv_link_if* recv_link)
    {
        /* Check address and if no match, return false */
        uint32_t* data = (uint32_t*)buff->data();
        if (data[ADDR_OFFSET] != _recv_addr) {
            return false;
        }
        if (data[TYPE_OFFSET] == 1) { /* Flow control message */
            _ackno = data[SEQNO_OFFSET];
        }
        if (data[TYPE_OFFSET] != 0) { /* Only data packets go up to user */
            recv_link->release_recv_buff(std::move(buff));
        } else { /* mock_send_transport does not receive data packets */
            return false;
        }
        return true;
    }

    std::pair<uint32_t*, size_t> buff_to_data(frame_buff* buff)
    {
        uint32_t* data  = (uint32_t*)buff->data();
        size_t data_len = buff->packet_size() - DATA_OFFSET * sizeof(uint32_t);
        return std::pair<uint32_t*, size_t>(&data[DATA_OFFSET], data_len);
    }

private:
    uint32_t _send_addr;
    uint32_t _recv_addr;
    uint32_t _credits;
    send_io_if::sptr _msg_if;
    send_io_if::sptr _send_if;
    uint32_t _seqno = 0;
    uint32_t _ackno = 0;
    size_t _frame_size;
};

/*!
 * Mock transport with following packet format:
 * Data: [dst_addr/src_addr, type, seqno, data_len, data...]
 * FC: [dst_addr/src_addr, type, ackno]
 * Msg: [dst_addr/src_addr, type, seqno, data_len, data...]
 * All fields are 32-bit words (dst_addr and src_addr are 16 bits each)
 */
class mock_recv_transport
{
public:
    using sptr = std::shared_ptr<mock_recv_transport>;

    mock_recv_transport(io_service::sptr io_srv,
        recv_link_if::sptr recv_link,
        send_link_if::sptr send_link,
        uint16_t dst_addr,
        uint16_t src_addr,
        uint32_t credits)
    {
        _send_addr = (src_addr << 16) | (dst_addr << 0);
        _recv_addr = (dst_addr << 16) | (src_addr << 0);

        /* Make client for sending streaming data */
        recv_io_if::fc_callback_t send_cb = [this](frame_buff::uptr buff,
                                                recv_link_if* recv_link,
                                                send_link_if* send_link) {
            this->handle_flow_ctrl(std::move(buff), recv_link, send_link);
        };
        recv_callback_t recv_cb = [this](frame_buff::uptr& buff,
                                      recv_link_if* link,
                                      send_link_if* /*send_link*/) {
            return this->recv_buff(buff, link);
        };
        /* Pretend get 1 flow control message per sent packet */
        _recv_if = io_srv->make_recv_client(
            recv_link, credits, recv_cb, send_link, credits, send_cb);
    }

    ~mock_recv_transport() {}

    /*!
     * Get a buffer for creating a non-flow-controlled message
     */
    bool get_msg(uint32_t& msg)
    {
        if (_msg_queue.read_available()) {
            msg = _msg_queue.front();
            _msg_queue.pop();
            return true;
        }
        return false;
    }

    /*!
      * Get an empty frame buffer in which to write packet contents.
      *
      * \param timeout_ms a positive timeout value specifies the maximum number
                          of ms to wait, a negative value specifies to block
                          until successful, and a value of 0 specifies no wait.
      * \return a frame buffer, or null uptr if timeout occurs
      */
    frame_buff::uptr get_data_buff(int32_t timeout_ms)
    {
        return _recv_if->get_recv_buff(timeout_ms);
    }

    /*!
     * Release a frame buffer, allowing the driver to reuse it.
     *
     * \param buffer frame buffer to release for reuse by the link
     */
    void release_data_buff(frame_buff::uptr buff)
    {
        _recv_if->release_recv_buff(std::move(buff));
    }

    /*!
     * Callback for producing a flow control response.
     * This callback is run whenever a frame_buff is scheduled to be released.
     *
     * The callback must release the buffer, but it can update internal state
     * as well. It can also send a response with the send_link_if, should it
     * desire to do so.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     */
    void handle_flow_ctrl(
        frame_buff::uptr buff, recv_link_if* recv_link, send_link_if* send_link)
    {
        uint32_t* data = (uint32_t*)buff->data();
        if (data[TYPE_OFFSET] == 0) {
            frame_buff::uptr fc_buff = send_link->get_send_buff(0);
            UHD_ASSERT_THROW(fc_buff);
            uint32_t* fc_data     = (uint32_t*)fc_buff->data();
            fc_data[SEQNO_OFFSET] = data[SEQNO_OFFSET];
            recv_link->release_recv_buff(std::move(buff));
            UHD_ASSERT_THROW(buff == nullptr);
            fc_data[TYPE_OFFSET] = 1; /* FC type */
            fc_data[ADDR_OFFSET] = _send_addr;
            fc_buff->set_packet_size(3 * sizeof(uint32_t));
            send_link->release_send_buff(std::move(fc_buff));
        } else {
            recv_link->release_recv_buff(std::move(buff));
        }
    }

    /*!
     * Callback for when packets are received (for processing).
     * Function should make a determination of whether the packet belongs to it
     * and return the bool.
     *
     * Function may consume and release the buffer internally (if packet was
     * destined for it). The recv_link_if may be used to release it, and the
     * provided frame_buff::uptr must relinquish ownership before returning.
     * If the buffer was not destined for the user of this function, buffer must
     * NOT be released, and the uptr must remain intact.
     *
     * Callbacks execute on the I/O thread! Be careful about what state is
     * touched. In addition, this callback should NOT sleep.
     *
     * \param frame_buff the buffer that was received
     * \param recv_link_if the link used to retrieve the buffer. Can be used to
     *  release the buffer back to the link, if buffer is consumed internally.
     * \return true if buffer matched this transport, false otherwise
     */
    bool recv_buff(frame_buff::uptr& buff, recv_link_if* recv_link)
    {
        /* Check address and if no match, return false */
        uint32_t* data = (uint32_t*)buff->data();
        if (data[ADDR_OFFSET] != _recv_addr) {
            return false;
        }
        if (data[TYPE_OFFSET] == 1) { /* No FC for mock_recv_transport */
            return false;
        }
        if (data[TYPE_OFFSET] == 2) { /* Record message */
            _msg_queue.push(data[DATA_OFFSET]);
            recv_link->release_recv_buff(std::move(buff));
        }
        /* (Data packets will go up to user) */
        return true;
    }

    std::pair<uint32_t*, size_t> buff_to_data(frame_buff* buff)
    {
        uint32_t* data  = (uint32_t*)buff->data();
        size_t data_len = data[LEN_OFFSET];
        return std::pair<uint32_t*, size_t>(&data[DATA_OFFSET], data_len);
    }

private:
    uint32_t _send_addr;
    uint32_t _recv_addr;
    recv_io_if::sptr _recv_if;
    boost::lockfree::spsc_queue<uint32_t, boost::lockfree::capacity<8>> _msg_queue;
};

}} // namespace uhd::transport

#endif /* INCLUDED_UHDLIB_TRANSPORT_TRANSPORT_IF_HPP */
