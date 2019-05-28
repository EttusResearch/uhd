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
        , _num_recv_frames(num_recv_frames)
        , _fc_link(fc_link)
        , _num_send_frames(num_send_frames)
        , _fc_cb(fc_cb)
    {
    }

    ~inline_recv_io()
    {
        _io_srv->disconnect_receiver(_data_link.get(), this, _num_recv_frames);
        if (_fc_link) {
            _io_srv->disconnect_sender(_fc_link.get(), _num_send_frames);
        }
    }

    frame_buff::uptr get_recv_buff(int32_t timeout_ms)
    {
        return _io_srv->recv(this, _data_link.get(), timeout_ms);
    }

    void release_recv_buff(frame_buff::uptr buff)
    {
        _fc_cb(frame_buff::uptr(std::move(buff)), _data_link.get(), _fc_link.get());
    }

private:
    inline_io_service::sptr _io_srv;
    recv_link_if::sptr _data_link;
    size_t _num_recv_frames;
    send_link_if::sptr _fc_link;
    size_t _num_send_frames;
    fc_callback_t _fc_cb;
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
        recv_callback_t fc_cb)
        : inline_recv_cb(fc_cb, send_link.get())
        , _io_srv(io_srv)
        , _send_link(send_link)
        , _num_send_frames(num_send_frames)
        , _send_cb(send_cb)
        , _recv_link(recv_link)
        , _num_recv_frames(num_recv_frames)
    {
    }

    ~inline_send_io()
    {
        _io_srv->disconnect_sender(_send_link.get(), _num_send_frames);
        if (_recv_link) {
            _io_srv->disconnect_receiver(_recv_link.get(), this, _num_recv_frames);
        }
    }

    frame_buff::uptr get_send_buff(int32_t timeout_ms)
    {
        /* Check initial flow control result */
        frame_buff::uptr buff = _send_link->get_send_buff(timeout_ms);
        if (buff) {
            return frame_buff::uptr(std::move(buff));
        }
        return frame_buff::uptr();
    }

    void release_send_buff(frame_buff::uptr buff)
    {
        while (buff) { /* TODO: Possibly don't loop indefinitely here */
            if (_recv_link) {
                _io_srv->recv(this, _recv_link.get(), 0);
            }
            _send_cb(buff, _send_link.get());
        }
    }

private:
    inline_io_service::sptr _io_srv;
    send_link_if::sptr _send_link;
    size_t _num_send_frames;
    send_callback_t _send_cb;
    recv_link_if::sptr _recv_link;
    size_t _num_recv_frames;
    recv_callback_t _recv_cb;
};

inline_io_service::~inline_io_service(){};

void inline_io_service::attach_recv_link(recv_link_if::sptr link)
{
    auto link_ptr = link.get();
    UHD_ASSERT_THROW(_recv_tbl.count(link_ptr) == 0);
    _recv_tbl[link_ptr] =
        std::tuple<inline_recv_mux*, inline_recv_cb*, size_t>(nullptr, nullptr, 0);
    _recv_links.push_back(link);
};

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
        UHD_ASSERT_THROW(num_send_frames > 0);
        UHD_ASSERT_THROW(fc_cb);
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
    auto link_ptr = link.get();
    UHD_ASSERT_THROW(_send_tbl.count(link_ptr) == 0);
    _send_tbl[link_ptr] = 0;
    _send_links.push_back(link);
};

send_io_if::sptr inline_io_service::make_send_client(send_link_if::sptr send_link,
    size_t num_send_frames,
    send_io_if::send_callback_t send_cb,
    recv_link_if::sptr recv_link,
    size_t num_recv_frames,
    recv_callback_t recv_cb)
{
    UHD_ASSERT_THROW(send_link);
    UHD_ASSERT_THROW(num_send_frames > 0);
    UHD_ASSERT_THROW(send_cb);
    connect_sender(send_link.get(), num_send_frames);
    sptr io_srv  = shared_from_this();
    auto send_io = std::make_shared<inline_send_io>(
        io_srv, send_link, num_send_frames, send_cb, recv_link, num_recv_frames, recv_cb);
    if (recv_link) {
        UHD_ASSERT_THROW(num_recv_frames > 0);
        UHD_ASSERT_THROW(recv_cb);
        connect_receiver(recv_link.get(), send_io.get(), num_recv_frames);
    }
    return send_io;
}

/*
 * Senders are free to mux a send_link, but the total reserved send_frames
 * must be less than or equal to the link's capacity
 */
void inline_io_service::connect_sender(send_link_if* link, size_t num_frames)
{
    size_t rsvd_frames    = _send_tbl.at(link);
    size_t frame_capacity = link->get_num_send_frames();
    UHD_ASSERT_THROW(frame_capacity >= rsvd_frames + num_frames);
    _send_tbl[link] = rsvd_frames + num_frames;
}

void inline_io_service::disconnect_sender(send_link_if* link, size_t num_frames)
{
    size_t rsvd_frames = _send_tbl.at(link);
    UHD_ASSERT_THROW(rsvd_frames >= num_frames);
    _send_tbl[link] = rsvd_frames - num_frames;
}

void inline_io_service::connect_receiver(
    recv_link_if* link, inline_recv_cb* cb, size_t num_frames)
{
    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    size_t rsvd_frames;
    std::tie(mux, rcvr, rsvd_frames) = _recv_tbl.at(link);
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
    size_t capacity = link->get_num_recv_frames();
    UHD_ASSERT_THROW(rsvd_frames + num_frames <= capacity);
    _recv_tbl[link] = std::make_tuple(mux, rcvr, rsvd_frames + num_frames);
}

void inline_io_service::disconnect_receiver(
    recv_link_if* link, inline_recv_cb* cb, size_t num_frames)
{
    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    size_t rsvd_frames;
    std::tie(mux, rcvr, rsvd_frames) = _recv_tbl.at(link);
    UHD_ASSERT_THROW(rsvd_frames >= num_frames);
    if (mux) {
        mux->disconnect(cb);
        if (mux->is_empty()) {
            delete mux;
            mux = nullptr;
        }
    } else {
        rcvr = nullptr;
    }
    _recv_tbl[link] = std::make_tuple(mux, rcvr, rsvd_frames - num_frames);
}

frame_buff::uptr inline_io_service::recv(
    inline_recv_cb* recv_io_cb, recv_link_if* recv_link, int32_t timeout_ms)
{
    inline_recv_mux* mux;
    inline_recv_cb* rcvr;
    size_t num_frames;
    std::tie(mux, rcvr, num_frames) = _recv_tbl.at(recv_link);

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
    return frame_buff::uptr();
}

}} // namespace uhd::transport
