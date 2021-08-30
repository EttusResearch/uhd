//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/inline_io_service.hpp>
#include <boost/circular_buffer.hpp>
#include <cassert>

namespace uhd { namespace transport {

/*!
 * Interface class for unifying callback processing between both inline_send_io
 * and inline_recv_io
 */
class inline_recv_cb
{
public:
    /*!
     * Function to call the callback method
     *
     * \param buff buffer received
     * \param recv_link pointer to recv link used with the callback
     * \return whether the packet was destined for this callback
     */
    UHD_FORCE_INLINE bool callback(frame_buff::uptr& buff, recv_link_if* recv_link)
    {
        return _recv_cb(buff, recv_link, _cb_send_link);
    }

protected:
    inline_recv_cb(recv_callback_t cb, send_link_if* send_link)
        : _recv_cb(cb), _cb_send_link(send_link)
    {
    }

    recv_callback_t _recv_cb;
    // pointer to send link used with the callback
    send_link_if* _cb_send_link;
};

/*!
 * Mux class that intercepts packets from the link and distributes them to
 * queues for each client that is not the caller of the recv() function
 */
class inline_recv_mux
{
public:
    inline_recv_mux(recv_link_if* link) : _link(link){};

    ~inline_recv_mux(){};

    /*!
     * Connect a new receiver to the recv link
     *
     * \param cb pointer to the callback for the receiver
     */
    void connect(inline_recv_cb* cb)
    {
        UHD_ASSERT_THROW(_queues.count(cb) == 0);
        /* Always create queue of max size, since we don't know when there are
         * virtual channels (which share frames)
         */
        auto queue =
            new boost::circular_buffer<frame_buff*>(_link->get_num_recv_frames());
        _queues[cb] = queue;
        _callbacks.push_back(cb);
    }

    /*!
     * Disconnect a receiver currently connected to the recv link
     * \param cb a pointer to the callback to disconnect
     */
    void disconnect(inline_recv_cb* cb)
    {
        auto queue = _queues.at(cb);
        while (!queue->empty()) {
            frame_buff* buff = queue->front();
            _link->release_recv_buff(frame_buff::uptr(buff));
            queue->pop_front();
        }
        delete queue;
        _queues.erase(cb);
        _callbacks.remove(cb);
    }

    /*!
     * Check if there are callbacks registered to this mux
     * \return whether there are no callbacks registered
     */
    UHD_FORCE_INLINE bool is_empty(void) const
    {
        return _callbacks.empty();
    }

    /*!
     * Do receive processing for the mux
     * \param cb the callback that is currently seeking a buffer
     * \param recv_link the link to do recv on
     * \param timeout_ms the timeout for the recv operation
     * \return a frame_buff with data if a packet was received (else empty)
     */
    frame_buff::uptr recv(inline_recv_cb* cb, recv_link_if* recv_link, int32_t timeout_ms)
    {
        auto queue = _queues.at(cb);
        if (!queue->empty()) {
            frame_buff* buff = queue->front();
            queue->pop_front();
            return frame_buff::uptr(buff);
        }
        while (true) {
            frame_buff::uptr buff = recv_link->get_recv_buff(timeout_ms);
            /* Process buffer */
            if (buff) {
                bool rcvr_found = false;
                for (auto& rcvr : _callbacks) {
                    if (rcvr->callback(buff, recv_link)) {
                        rcvr_found = true;
                        if (buff) {
                            if (rcvr == cb) {
                                return frame_buff::uptr(std::move(buff));
                            } else {
                                /* NOTE: Should not overflow, by construction
                                 * Every queue can hold link->get_num_recv_frames()
                                 */
                                _queues[rcvr]->push_back(buff.release());
                            }
                        }
                        /* Continue looping if buffer was consumed */
                        break;
                    }
                }
                if (not rcvr_found) {
                    UHD_LOG_DEBUG("IO_SRV", "Dropping packet with no receiver");
                    recv_link->release_recv_buff(std::move(buff));
                }
            } else { /* Timeout */
                return frame_buff::uptr();
            }
        }
    }

    bool recv_flow_ctrl(inline_recv_cb* cb, recv_link_if* recv_link, int32_t timeout_ms)
    {
        while (true) {
            frame_buff::uptr buff = recv_link->get_recv_buff(timeout_ms);
            /* Process buffer */
            if (buff) {
                bool rcvr_found = false;
                for (auto& rcvr : _callbacks) {
                    if (rcvr->callback(buff, recv_link)) {
                        rcvr_found = true;
                        if (rcvr == cb) {
                            assert(!buff);
                            return true;
                        } else if (buff) {
                            /* NOTE: Should not overflow, by construction
                             * Every queue can hold link->get_num_recv_frames()
                             */
                            _queues[rcvr]->push_back(buff.release());
                        } else {
                            /* Continue looping if buffer was consumed and
                               receiver is not the requested one */
                            break;
                        }
                    }
                }
                if (not rcvr_found) {
                    UHD_LOG_DEBUG("IO_SRV", "Dropping packet with no receiver");
                    recv_link->release_recv_buff(std::move(buff));
                }
            } else { /* Timeout */
                return false;
            }
        }
    }

private:
    recv_link_if* _link;
    std::list<inline_recv_cb*> _callbacks;
    std::unordered_map<inline_recv_cb*, boost::circular_buffer<frame_buff*>*> _queues;
};

class inline_recv_io : public virtual recv_io_if, public virtual inline_recv_cb
{
public:
    using sptr = std::shared_ptr<inline_recv_io>;

    inline_recv_io(inline_io_service::sptr io_srv,
        recv_link_if::sptr data_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        send_link_if::sptr fc_link,
        size_t num_send_frames,
        fc_callback_t fc_cb)
        : inline_recv_cb(recv_cb, fc_link.get())
        , _io_srv(io_srv)
        , _data_link(data_link)
        , _fc_link(fc_link)
        , _fc_cb(fc_cb)
    {
        _num_recv_frames = num_recv_frames;
        _num_send_frames = num_send_frames;
    }

    ~inline_recv_io() override
    {
        _io_srv->disconnect_receiver(_data_link.get(), this);
        if (_fc_link) {
            _io_srv->disconnect_sender(_fc_link.get());
        }
    }

    frame_buff::uptr get_recv_buff(int32_t timeout_ms) override
    {
        auto buff = _io_srv->recv(this, _data_link.get(), timeout_ms);
        if (buff) {
            _num_frames_in_use++;
            assert(_num_frames_in_use <= _num_recv_frames);
        }
        return buff;
    }

    void release_recv_buff(frame_buff::uptr buff) override
    {
        _fc_cb(frame_buff::uptr(std::move(buff)), _data_link.get(), _fc_link.get());
        _num_frames_in_use--;
    }

private:
    inline_io_service::sptr _io_srv;
    recv_link_if::sptr _data_link;
    send_link_if::sptr _fc_link;
    fc_callback_t _fc_cb;
    size_t _num_frames_in_use = 0;
};

class inline_send_io : public virtual send_io_if, public virtual inline_recv_cb
{
public:
    using sptr = std::shared_ptr<inline_send_io>;

    inline_send_io(inline_io_service::sptr io_srv,
        send_link_if::sptr send_link,
        size_t num_send_frames,
        send_callback_t send_cb,
        recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        send_io_if::fc_callback_t fc_cb)
        : inline_recv_cb(recv_cb, send_link.get())
        , _io_srv(io_srv)
        , _send_link(send_link)
        , _send_cb(send_cb)
        , _recv_link(recv_link)
        , _fc_cb(fc_cb)
    {
        _num_recv_frames = num_recv_frames;
        _num_send_frames = num_send_frames;
    }

    ~inline_send_io() override
    {
        _io_srv->disconnect_sender(_send_link.get());
        if (_recv_link) {
            _io_srv->disconnect_receiver(_recv_link.get(), this);
        }
    }

    bool wait_for_dest_ready(size_t num_bytes, int32_t timeout_ms) override
    {
        if (!_recv_link) {
            // If there is no flow control link, then the destination must
            // always be ready for more data.
            return true;
        }

        while (!_fc_cb(num_bytes)) {
            const bool updated =
                _io_srv->recv_flow_ctrl(this, _recv_link.get(), timeout_ms);

            if (!updated) {
                return false;
            }
        }
        return true;
    }

    frame_buff::uptr get_send_buff(int32_t timeout_ms) override
    {
        frame_buff::uptr buff = _send_link->get_send_buff(timeout_ms);
        if (buff) {
            _num_frames_in_use++;
            assert(_num_frames_in_use <= _num_send_frames);
            return frame_buff::uptr(std::move(buff));
        }
        return frame_buff::uptr();
    }

    void release_send_buff(frame_buff::uptr buff) override
    {
        // Send the packet using callback
        _send_cb(std::move(buff), _send_link.get());
        _num_frames_in_use--;
    }

private:
    inline_io_service::sptr _io_srv;
    send_link_if::sptr _send_link;
    send_callback_t _send_cb;
    recv_link_if::sptr _recv_link;
    recv_callback_t _recv_cb;
    fc_callback_t _fc_cb;
    size_t _num_frames_in_use = 0;
};

inline_io_service::~inline_io_service(){};

void inline_io_service::attach_recv_link(recv_link_if::sptr link)
{
    auto link_ptr = link.get();
    UHD_ASSERT_THROW(_recv_tbl.count(link_ptr) == 0);
    _recv_tbl[link_ptr] = std::tuple<inline_recv_mux*, inline_recv_cb*>(nullptr, nullptr);
    _recv_links.push_back(link);
}

void inline_io_service::detach_recv_link(recv_link_if::sptr link)
{
    auto link_ptr = link.get();
    UHD_ASSERT_THROW(_recv_tbl.count(link_ptr) != 0);
    _recv_tbl.erase(link_ptr);

    _recv_links.remove_if(
        [link_ptr](recv_link_if::sptr& item) { return item.get() == link_ptr; });
}

recv_io_if::sptr inline_io_service::make_recv_client(recv_link_if::sptr data_link,
    size_t num_recv_frames,
    recv_callback_t cb,
    send_link_if::sptr fc_link,
    size_t num_send_frames,
    recv_io_if::fc_callback_t fc_cb)
{
    UHD_ASSERT_THROW(data_link);
    UHD_ASSERT_THROW(num_recv_frames > 0);
    UHD_ASSERT_THROW(cb);
    if (fc_link) {
        UHD_ASSERT_THROW(fc_cb);
        UHD_ASSERT_THROW(num_send_frames > 0);
        connect_sender(fc_link.get(), num_send_frames);
    }
    sptr io_srv  = shared_from_this();
    auto recv_io = std::make_shared<inline_recv_io>(
        io_srv, data_link, num_recv_frames, cb, fc_link, num_send_frames, fc_cb);
    connect_receiver(data_link.get(), recv_io.get(), num_recv_frames);
    return recv_io;
}

void inline_io_service::attach_send_link(send_link_if::sptr link)
{
    UHD_ASSERT_THROW(
        std::find(_send_links.begin(), _send_links.end(), link) == _send_links.end());
    _send_links.push_back(link);
}

void inline_io_service::detach_send_link(send_link_if::sptr link)
{
    auto link_ptr = link.get();
    _send_links.remove_if(
        [link_ptr](send_link_if::sptr& item) { return item.get() == link_ptr; });
}

send_io_if::sptr inline_io_service::make_send_client(send_link_if::sptr send_link,
    size_t num_send_frames,
    send_io_if::send_callback_t send_cb,
    recv_link_if::sptr recv_link,
    size_t num_recv_frames,
    recv_callback_t recv_cb,
    send_io_if::fc_callback_t fc_cb)
{
    UHD_ASSERT_THROW(send_link);
    UHD_ASSERT_THROW(num_send_frames > 0);
    UHD_ASSERT_THROW(send_cb);
    connect_sender(send_link.get(), num_send_frames);
    sptr io_srv  = shared_from_this();
    auto send_io = std::make_shared<inline_send_io>(io_srv,
        send_link,
        num_send_frames,
        send_cb,
        recv_link,
        num_recv_frames,
        recv_cb,
        fc_cb);
    if (recv_link) {
        UHD_ASSERT_THROW(recv_cb);
        UHD_ASSERT_THROW(fc_cb);
        UHD_ASSERT_THROW(num_recv_frames > 0);
        connect_receiver(recv_link.get(), send_io.get(), num_recv_frames);
    }
    return send_io;
}

/*
 * This I/O service does not check frame reservations strictly since frames can
 * be shared by multiple clients as long as they are not in use at the same
 * time.
 */
void inline_io_service::connect_sender(send_link_if* link, size_t num_frames)
{
    size_t frame_capacity = link->get_num_send_frames();
    UHD_ASSERT_THROW(frame_capacity >= num_frames);
}

void inline_io_service::disconnect_sender(send_link_if* /*link*/)
{
    // No-op
}

void inline_io_service::connect_receiver(
    recv_link_if* link, inline_recv_cb* cb, size_t num_frames)
{
    size_t capacity = link->get_num_recv_frames();
    UHD_ASSERT_THROW(num_frames <= capacity);

    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    std::tie(mux, rcvr) = _recv_tbl.at(link);
    if (mux) {
        mux->connect(cb);
    } else if (rcvr) {
        mux = new inline_recv_mux(link);
        mux->connect(rcvr);
        mux->connect(cb);
        rcvr = nullptr;
    } else {
        rcvr = cb;
    }
    _recv_tbl[link] = std::make_tuple(mux, rcvr);
}

void inline_io_service::disconnect_receiver(recv_link_if* link, inline_recv_cb* cb)
{
    inline_recv_mux* mux = nullptr;
    inline_recv_cb* rcvr = nullptr;
    if (_recv_tbl.count(link)) {
        std::tie(mux, rcvr) = _recv_tbl.at(link);
        if (mux) {
            mux->disconnect(cb);
            if (mux->is_empty()) {
                delete mux;
                mux = nullptr;
            }
        }
    }
    _recv_tbl[link] = std::make_tuple(mux, rcvr);
}

frame_buff::uptr inline_io_service::recv(
    inline_recv_cb* recv_io_cb, recv_link_if* recv_link, int32_t timeout_ms)
{
    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    std::tie(mux, rcvr) = _recv_tbl.at(recv_link);

    if (mux) {
        /* Defer to mux's recv() if present */
        return mux->recv(recv_io_cb, recv_link, timeout_ms);
    } else {
        assert(recv_io_cb == rcvr);
    }

    while (true) {
        frame_buff::uptr buff = recv_link->get_recv_buff(timeout_ms);
        /* Process buffer */
        if (buff) {
            if (rcvr->callback(buff, recv_link)) {
                if (buff) {
                    return frame_buff::uptr(std::move(buff));
                }
                /* Retry receive if got buffer but it got consumed */
            } else {
                UHD_LOG_DEBUG("IO_SRV", "Dropping packet with no receiver");
                recv_link->release_recv_buff(std::move(buff));
            }
        } else { /* Timeout */
            return frame_buff::uptr();
        }
    }
}

bool inline_io_service::recv_flow_ctrl(
    inline_recv_cb* recv_io_cb, recv_link_if* recv_link, int32_t timeout_ms)
{
    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    std::tie(mux, rcvr) = _recv_tbl.at(recv_link);

    if (mux) {
        /* Defer to mux's recv_flow_ctrl() if present */
        return mux->recv_flow_ctrl(recv_io_cb, recv_link, timeout_ms);
    } else {
        assert(recv_io_cb == rcvr);
    }

    while (true) {
        frame_buff::uptr buff = recv_link->get_recv_buff(timeout_ms);
        /* Process buffer */
        if (buff) {
            if (rcvr->callback(buff, recv_link)) {
                assert(!buff);
                return true;
            } else {
                UHD_LOG_DEBUG("IO_SRV", "Dropping packet with no receiver");
                recv_link->release_recv_buff(std::move(buff));
            }
        } else { /* Timeout */
            return false;
        }
    }
}

}} // namespace uhd::transport
