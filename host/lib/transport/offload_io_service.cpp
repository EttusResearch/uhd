//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/thread.hpp>
#include <uhdlib/transport/offload_io_service.hpp>
#include <condition_variable>
#include <unordered_map>
#include <boost/lockfree/queue.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <thread>

namespace uhd { namespace transport {

namespace {

constexpr int32_t blocking_timeout_ms = 100;

// Struct to help keep track of frames reserved for each link
struct frame_reservation_t
{
    recv_link_if::sptr recv_link;
    size_t num_recv_frames = 0;
    send_link_if::sptr send_link;
    size_t num_send_frames = 0;
};

// Helper class to keep track of frames reserved for each link
class frame_reservation_mgr
{
public:
    void register_link(const recv_link_if::sptr& recv_link)
    {
        if (_recv_tbl[recv_link.get()] != 0) {
            throw uhd::runtime_error("Recv link already attached to I/O service");
        }
        _recv_tbl[recv_link.get()] = 0;
    }

    void register_link(const send_link_if::sptr& send_link)
    {
        if (_send_tbl[send_link.get()] != 0) {
            throw uhd::runtime_error("Send link already attached to I/O service");
        }
        _send_tbl[send_link.get()] = 0;
    }

    void reserve_frames(const frame_reservation_t& reservation)
    {
        if (reservation.recv_link) {
            const size_t rsvd_frames = _recv_tbl.at(reservation.recv_link.get());
            const size_t capacity    = reservation.recv_link->get_num_recv_frames();
            if (rsvd_frames + reservation.num_recv_frames > capacity) {
                throw uhd::runtime_error("Number of frames requested exceeds link recv frame capacity");
            }

            recv_link_if* link_ptr = reservation.recv_link.get();
            _recv_tbl[link_ptr]    = rsvd_frames + reservation.num_recv_frames;
        }

        if (reservation.send_link) {
            const size_t rsvd_frames = _send_tbl.at(reservation.send_link.get());
            const size_t capacity    = reservation.send_link->get_num_send_frames();
            if (rsvd_frames + reservation.num_send_frames > capacity) {
                throw uhd::runtime_error("Number of frames requested exceeds link send frame capacity");
            }

            send_link_if* link_ptr = reservation.send_link.get();
            _send_tbl[link_ptr]    = rsvd_frames + reservation.num_send_frames;
        }
    }

    void unreserve_frames(const frame_reservation_t& reservation)
    {
        if (reservation.recv_link) {
            const size_t rsvd_frames = _recv_tbl.at(reservation.recv_link.get());
            recv_link_if* link_ptr   = reservation.recv_link.get();
            _recv_tbl[link_ptr]      = rsvd_frames - reservation.num_recv_frames;
        }

        if (reservation.send_link) {
            const size_t rsvd_frames = _send_tbl.at(reservation.send_link.get());
            send_link_if* link_ptr   = reservation.send_link.get();
            _send_tbl[link_ptr]      = rsvd_frames - reservation.num_send_frames;
        }
    }

private:
    std::unordered_map<recv_link_if*, size_t> _recv_tbl;
    std::unordered_map<send_link_if*, size_t> _send_tbl;
};


// Semaphore used in blocking I/O for offload thread
class semaphore
{
public:
    void notify() {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        _count++;
        _cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        _cv.wait(lock, [this]() { return this->_count != 0; });
        _count--;
    }

    bool try_wait() {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        if (_count != 0) {
            _count--;
            return true;
        }
        return false;
    }

    bool wait_for(size_t timeout_ms) {
        std::chrono::milliseconds timeout(timeout_ms);
        std::unique_lock<std::mutex> lock(_cv_mutex);
        if (_cv.wait_for(lock, timeout, [this]() { return this->_count != 0; })) {
            _count--;
            return true;
        }
        return false;
    }

    size_t count() {
        std::unique_lock<std::mutex> lock(_cv_mutex);
        return _count;
    }

private:
    std::condition_variable _cv;
    std::mutex _cv_mutex;
    size_t _count = 0;
};

// Fixed-size queue that supports blocking semantics
template <typename queue_item_t>
class offload_thread_queue {
public:
    offload_thread_queue(size_t size)
        : _buffer(new queue_item_t[size])
        , _capacity(size)
    {
    }

    ~offload_thread_queue()
    {
        delete [] _buffer;
    }

    void push(const queue_item_t& item)
    {
        _buffer[_write_index++] = item;
        _write_index %= _capacity;
        _item_sem.notify();
    }

    bool pop(queue_item_t& item)
    {
        if (_item_sem.try_wait()) {
            item = _buffer[_read_index++];
            _read_index %= _capacity;
            return true;
        } else {
            return false;
        }
    }

    bool pop(queue_item_t& item, int32_t timeout_ms)
    {
        if (_item_sem.wait_for(timeout_ms)) {
            item = _buffer[_read_index++];
            _read_index %= _capacity;
            return true;
        } else {
            return false;
        }
    }

    size_t read_available()
    {
        return _item_sem.count();
    }

private:
    queue_item_t* _buffer;
    const size_t _capacity;

    size_t _read_index  = 0;
    size_t _write_index = 0;

    // Semaphore gating number of items available to read
    semaphore _item_sem;
};

// Object that implements the communication between client and offload thread
struct client_port_t
{
public:
    using sptr = std::shared_ptr<client_port_t>;

    client_port_t(size_t size)
        : _from_offload_thread(size)
        , _to_offload_thread(size + 1) // add one for disconnect command
    {
    }

    //
    // Client methods
    //
    frame_buff* client_pop()
    {
        from_offload_thread_t queue_element;
        _from_offload_thread.pop(queue_element);
        return queue_element.buff;
    }

    size_t client_read_available()
    {
        return _from_offload_thread.read_available();
    }

    void client_push(frame_buff* buff)
    {
        to_offload_thread_t queue_element{buff, false};
        _to_offload_thread.push(queue_element);
    }

    void client_wait_until_connected()
    {
        std::unique_lock<std::mutex> lock(_connect_cv_mutex);
        _connect_cv.wait(lock, [this]() { return _connected; });
    }

    void client_disconnect()
    {
        to_offload_thread_t queue_element{nullptr, true};
        _to_offload_thread.push(queue_element);

        // Need to wait for the disconnect to occur before returning, since the
        // caller (the xport object) has callbacks installed in the inline I/O
        // service. After this method returns, the caller can be deallocated.
        std::unique_lock<std::mutex> lock(_connect_cv_mutex);
        _connect_cv.wait(lock, [this]() { return !_connected; });
    }

    //
    // Offload thread methods
    //
    void offload_thread_push(frame_buff* buff)
    {
        from_offload_thread_t queue_element{buff};
        _from_offload_thread.push(queue_element);
    }

    std::tuple<frame_buff*, bool> offload_thread_pop()
    {
        to_offload_thread_t queue_element;
        _to_offload_thread.pop(queue_element);
        return std::make_tuple(queue_element.buff, queue_element.disconnect);
    }

    std::tuple<frame_buff*, bool> offload_thread_pop(int32_t timeout_ms)
    {
        to_offload_thread_t queue_element;
        _to_offload_thread.pop(queue_element, timeout_ms);
        return std::make_tuple(queue_element.buff, queue_element.disconnect);
    }

    void offload_thread_set_connected(const bool value)
    {
        {
            std::lock_guard<std::mutex> lock(_connect_cv_mutex);
            _connected = value;
        }
        _connect_cv.notify_one();
    }

    // Flush should only be called once the client is no longer accessing the
    // queue going from the offload thread to the client, since it drains that
    // queue from the offload thread.
    template <typename fn_t>
    size_t offload_thread_flush(fn_t f)
    {
        size_t count = 0;
        from_offload_thread_t queue_element;
        while (_from_offload_thread.pop(queue_element)) {
            f(queue_element.buff);
            count++;
        }
        return count;
    }

private:
    // Queue for frame buffers coming from the offload thread
    struct from_offload_thread_t
    {
        frame_buff* buff = nullptr;
    };

    using from_offload_thread_queue_t = offload_thread_queue<from_offload_thread_t>;

    // Queue for frame buffers and disconnect requests to offload thread. Disconnect
    // requests must be inline with incoming buffers to avoid any race conditions
    // between the two.
    struct to_offload_thread_t
    {
        frame_buff* buff = nullptr;
        bool disconnect  = false;
    };

    using to_offload_thread_queue_t = offload_thread_queue<to_offload_thread_t>;

    // Queues to carry frame buffers in both directions
    from_offload_thread_queue_t _from_offload_thread;
    to_offload_thread_queue_t _to_offload_thread;

    // Mutex and condition variable to wait for connect and disconnect
    std::condition_variable _connect_cv;
    std::mutex _connect_cv_mutex;
    bool _connected = false;
};

} // namespace

// Implementation of io service that executes an inline io service in an offload
// thread. The offload thread communicates with send and recv clients using a
// pair of spsc queues. One queue carries buffers from the offload thread to the
// client, and the other carries buffers in the opposite direction.
//
// Requests to create new clients are handled using a separate mpsc queue. Client
// requests to disconnect are sent in the same spsc queue as the buffers so that
// they are processed only after all buffer release requestss have been processed.
class offload_io_service_impl
    : public offload_io_service,
      public std::enable_shared_from_this<offload_io_service_impl>
{
public:
    using sptr = std::shared_ptr<offload_io_service_impl>;

    offload_io_service_impl(
        io_service::sptr io_srv, const offload_io_service::params_t& params);
    ~offload_io_service_impl();

    void attach_recv_link(recv_link_if::sptr link);
    void attach_send_link(send_link_if::sptr link);

    recv_io_if::sptr make_recv_client(recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t cb,
        send_link_if::sptr fc_link,
        size_t num_send_frames,
        recv_io_if::fc_callback_t fc_cb);

    send_io_if::sptr make_send_client(send_link_if::sptr send_link,
        size_t num_send_frames,
        send_io_if::send_callback_t send_cb,
        recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb);

private:
    offload_io_service_impl(const offload_io_service_impl&) = delete;

    // Queue for new client creation, multiple producers allowed. Requests are
    // passed as heap-allocated pointers because boost lockfree queues require
    // simple types.
    struct client_req_t
    {
        std::function<void()>* req = nullptr;
    };
    using client_req_queue_t = boost::lockfree::queue<client_req_t>;

    // Values used by offload thread for each client
    struct recv_client_info_t
    {
        client_port_t::sptr port;
        recv_io_if::sptr inline_io;
        size_t num_frames_in_use = 0;
        frame_reservation_t frames_reserved;
    };
    struct send_client_info_t
    {
        client_port_t::sptr port;
        send_io_if::sptr inline_io;
        size_t num_frames_in_use = 0;
        frame_reservation_t frames_reserved;
    };

    void _get_recv_buff(recv_client_info_t& info, int32_t timeout_ms);
    void _get_send_buff(send_client_info_t& info);
    void _release_recv_buff(recv_client_info_t& info, frame_buff* buff);
    void _release_send_buff(send_client_info_t& info, frame_buff* buff);
    void _disconnect_recv_client(recv_client_info_t& info);
    void _disconnect_send_client(send_client_info_t& info);

    template <bool allow_recv, bool allow_send>
    void _do_work_polling();

    template <bool allow_recv, bool allow_send>
    void _do_work_blocking();

    // The I/O service that executes within the offload thread
    io_service::sptr _io_srv;

    // Type of clients supported by this I/O service
    client_type_t _client_type;

    // Offload thread, its stop flag, and thread-related parameters
    std::unique_ptr<std::thread> _offload_thread;
    std::atomic<bool> _stop_offload_thread{false};
    offload_io_service::params_t _offload_thread_params;

    // Lists of clients and their respective queues
    std::list<recv_client_info_t> _recv_clients;
    std::list<send_client_info_t> _send_clients;

    // Queue for connect and disconnect client requests
    client_req_queue_t _client_connect_queue;

    // Keep track of frame reservations
    frame_reservation_mgr _reservation_mgr;
};

class offload_recv_io : public recv_io_if
{
public:
    offload_recv_io(offload_io_service_impl::sptr io_srv,
        size_t num_recv_frames,
        size_t num_send_frames,
        client_port_t::sptr& port);

    ~offload_recv_io();

    frame_buff::uptr get_recv_buff(int32_t timeout_ms);
    void release_recv_buff(frame_buff::uptr buff);

private:
    offload_recv_io()                       = delete;
    offload_recv_io(const offload_recv_io&) = delete;

    offload_io_service_impl::sptr _io_srv;
    client_port_t::sptr _port;
    size_t _num_frames_in_use = 0;
};

class offload_send_io : public send_io_if
{
public:
    offload_send_io(offload_io_service_impl::sptr io_srv,
        size_t num_recv_frames,
        size_t num_send_frames,
        client_port_t::sptr& port);

    ~offload_send_io();

    frame_buff::uptr get_send_buff(int32_t timeout_ms);
    void release_send_buff(frame_buff::uptr buff);

private:
    offload_send_io()                       = delete;
    offload_send_io(const offload_send_io&) = delete;

    offload_io_service_impl::sptr _io_srv;
    client_port_t::sptr _port;
    size_t _num_frames_in_use = 0;
};

// Implementation of get_send_buff used below by send and recv clients
template <typename pop_func_t>
static frame_buff::uptr client_get_buff(pop_func_t pop, const int32_t timeout_ms)
{
    using namespace std::chrono;

    if (timeout_ms == 0) {
        return frame_buff::uptr(pop());
    }

    const auto end_time = steady_clock::now() + milliseconds(timeout_ms);

    bool last_check = false;

    while (true) {
        if (frame_buff* buff = pop()) {
            return frame_buff::uptr(buff);
        }

        if (timeout_ms > 0 && steady_clock::now() > end_time) {
            if (last_check) {
                return nullptr;
            } else {
                last_check = true;
            }
        }
        std::this_thread::yield();
    }
}

//
// offload_recv_io methods
//
offload_recv_io::offload_recv_io(offload_io_service_impl::sptr io_srv,
    size_t num_recv_frames,
    size_t num_send_frames,
    client_port_t::sptr& port)
    : _io_srv(io_srv), _port(port)
{
    _num_recv_frames = num_recv_frames;
    _num_send_frames = num_send_frames;
}

offload_recv_io::~offload_recv_io()
{
    assert(_num_frames_in_use == 0);

    if (_io_srv) {
        _port->client_disconnect();
    }
}

frame_buff::uptr offload_recv_io::get_recv_buff(int32_t timeout_ms)
{
    return client_get_buff(
        [this]() {
            frame_buff* buff = _port->client_pop();
            _num_frames_in_use += buff ? 1 : 0;
            return buff;
        },
        timeout_ms);
}

void offload_recv_io::release_recv_buff(frame_buff::uptr buff)
{
    assert(buff);
    _port->client_push(buff.release());
    _num_frames_in_use--;
}

//
// offload_send_io methods
//
offload_send_io::offload_send_io(offload_io_service_impl::sptr io_srv,
    size_t num_recv_frames,
    size_t num_send_frames,
    client_port_t::sptr& port)
    : _io_srv(io_srv), _port(port)
{
    _num_recv_frames = num_recv_frames;
    _num_send_frames = num_send_frames;
}

offload_send_io::~offload_send_io()
{
    assert(_num_frames_in_use == 0);

    if (_io_srv) {
        _port->client_disconnect();
    }
}

frame_buff::uptr offload_send_io::get_send_buff(int32_t timeout_ms)
{
    return client_get_buff(
        [this]() {
            frame_buff* buff = _port->client_pop();
            _num_frames_in_use += buff ? 1 : 0;
            return buff;
        },
        timeout_ms);
}

void offload_send_io::release_send_buff(frame_buff::uptr buff)
{
    assert(buff);
    _port->client_push(buff.release());
    _num_frames_in_use--;
}

//
// offload_io_service methods
//
offload_io_service::sptr offload_io_service::make(
    io_service::sptr io_srv, const offload_io_service::params_t& params)
{
    return std::make_shared<offload_io_service_impl>(io_srv, params);
}

//
// offload_io_service_impl methods
//
offload_io_service_impl::offload_io_service_impl(
    io_service::sptr io_srv, const offload_io_service::params_t& params)
    : _io_srv(io_srv)
    , _offload_thread_params(params)
    , _client_connect_queue(10) // arbitrary initial size
{
    if (params.wait_mode == BLOCK && params.client_type == BOTH_SEND_AND_RECV) {
        throw uhd::value_error(
            "An I/O service configured to block should only service either "
            "send or recv clients to prevent one client type from starving "
            "the other");
    }

    std::function<void()> thread_fn;

    if (params.wait_mode == BLOCK) {
        if (params.client_type == RECV_ONLY) {
            thread_fn = [this]() { _do_work_blocking<true, false>(); };
        } else if (params.client_type == SEND_ONLY) {
            thread_fn = [this]() { _do_work_blocking<false, true>(); };
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    } else if (params.wait_mode == POLL) {
        if (params.client_type == RECV_ONLY) {
            thread_fn = [this]() { _do_work_polling<true, false>(); };
        } else if (params.client_type == SEND_ONLY) {
            thread_fn = [this]() { _do_work_polling<false, true>(); };
        } else if (params.client_type == BOTH_SEND_AND_RECV) {
            thread_fn = [this]() { _do_work_polling<true, true>(); };
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }

    _offload_thread = std::make_unique<std::thread>(thread_fn);
}

offload_io_service_impl::~offload_io_service_impl()
{
    _stop_offload_thread = true;

    if (_offload_thread) {
        _offload_thread->join();
    }

    assert(_recv_clients.empty());
    assert(_send_clients.empty());
}

void offload_io_service_impl::attach_recv_link(recv_link_if::sptr link)
{
    // Create a request to attach link in the offload thread
    auto req_fn = [this, link]() {
        _reservation_mgr.register_link(link);
        _io_srv->attach_recv_link(link);
    };

    client_req_t queue_element;
    queue_element.req  = {new std::function<void()>(req_fn)};
    const bool success = _client_connect_queue.push(queue_element);
    if (!success) {
        throw uhd::runtime_error("Failed to push attach_recv_link request");
    }
}

void offload_io_service_impl::attach_send_link(send_link_if::sptr link)
{
    // Create a request to attach link in the offload thread
    auto req_fn = [this, link]() {
        _reservation_mgr.register_link(link);
        _io_srv->attach_send_link(link);
    };

    client_req_t queue_element;
    queue_element.req  = {new std::function<void()>(req_fn)};
    const bool success = _client_connect_queue.push(queue_element);
    if (!success) {
        throw uhd::runtime_error("Failed to push attach_send_link request");
    }
}

recv_io_if::sptr offload_io_service_impl::make_recv_client(recv_link_if::sptr recv_link,
    size_t num_recv_frames,
    recv_callback_t cb,
    send_link_if::sptr fc_link,
    size_t num_send_frames,
    recv_io_if::fc_callback_t fc_cb)
{
    UHD_ASSERT_THROW(_offload_thread);

    if (_client_type == SEND_ONLY) {
        throw uhd::runtime_error("Recv client not supported by this I/O service");
    }

    auto port = std::make_shared<client_port_t>(num_recv_frames);

    // Create a request to create a new receiver in the offload thread
    auto req_fn =
        [this, recv_link, num_recv_frames, cb, fc_link, num_send_frames, fc_cb, port]() {
            frame_reservation_t frames = {recv_link, num_recv_frames, fc_link, num_send_frames};
            _reservation_mgr.reserve_frames(frames);

            auto inline_recv_io = _io_srv->make_recv_client(
                recv_link, num_recv_frames, cb, fc_link, num_send_frames, fc_cb);

            recv_client_info_t client_info;
            client_info.inline_io       = inline_recv_io;
            client_info.port            = port;
            client_info.frames_reserved = frames;

            _recv_clients.push_back(client_info);

            // Notify that the connection is created
            port->offload_thread_set_connected(true);
        };

    client_req_t queue_element;
    queue_element.req  = {new std::function<void()>(req_fn)};
    const bool success = _client_connect_queue.push(queue_element);
    if (!success) {
        throw uhd::runtime_error("Failed to push make_recv_client request");
    }

    port->client_wait_until_connected();

    // Return a new recv client to the caller that just operates on the queues
    return std::make_shared<offload_recv_io>(
        shared_from_this(), num_recv_frames, num_send_frames, port);
}

send_io_if::sptr offload_io_service_impl::make_send_client(send_link_if::sptr send_link,
    size_t num_send_frames,
    send_io_if::send_callback_t send_cb,
    recv_link_if::sptr recv_link,
    size_t num_recv_frames,
    recv_callback_t recv_cb)
{
    UHD_ASSERT_THROW(_offload_thread);

    if (_client_type == RECV_ONLY) {
        throw uhd::runtime_error("Send client not supported by this I/O service");
    }

    auto port = std::make_shared<client_port_t>(num_send_frames);

    // Create a request to create a new receiver in the offload thread
    auto req_fn = [this,
                      send_link,
                      num_send_frames,
                      send_cb,
                      recv_link,
                      num_recv_frames,
                      recv_cb,
                      port]() {
        frame_reservation_t frames = {recv_link, num_recv_frames, send_link, num_send_frames};
        _reservation_mgr.reserve_frames(frames);

        auto inline_send_io = _io_srv->make_send_client(
            send_link, num_send_frames, send_cb, recv_link, num_recv_frames, recv_cb);

        send_client_info_t client_info;
        client_info.inline_io       = inline_send_io;
        client_info.port            = port;
        client_info.frames_reserved = frames;

        _send_clients.push_back(client_info);

        // Notify that the connection is created
        port->offload_thread_set_connected(true);
    };

    client_req_t queue_element;
    queue_element.req  = {new std::function<void()>(req_fn)};
    const bool success = _client_connect_queue.push(queue_element);
    if (!success) {
        throw uhd::runtime_error("Failed to push make_send_client request");
    }

    port->client_wait_until_connected();

    // Wait for buffer queue to be full
    while (port->client_read_available() != num_send_frames) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Return a new recv client to the caller that just operates on the queues
    return std::make_shared<offload_send_io>(
        shared_from_this(), num_recv_frames, num_send_frames, port);
}

// Get a single receive buffer if available and update client info
void offload_io_service_impl::_get_recv_buff(recv_client_info_t& info, int32_t timeout_ms)
{
    if (info.num_frames_in_use < info.frames_reserved.num_recv_frames) {
        if (frame_buff::uptr buff = info.inline_io->get_recv_buff(timeout_ms)) {
            info.port->offload_thread_push(buff.release());
            info.num_frames_in_use++;
        }
    }
}

// Get a single send buffer if available and update client info
void offload_io_service_impl::_get_send_buff(send_client_info_t& info)
{
    if (info.num_frames_in_use < info.frames_reserved.num_send_frames) {
        if (frame_buff::uptr buff = info.inline_io->get_send_buff(0)) {
            info.port->offload_thread_push(buff.release());
            info.num_frames_in_use++;
        }
    }
}

// Release a single recv buffer and update client info
void offload_io_service_impl::_release_recv_buff(recv_client_info_t& info, frame_buff* buff)
{
    info.inline_io->release_recv_buff(frame_buff::uptr(buff));
    assert(info.num_frames_in_use > 0);
    info.num_frames_in_use--;
}

// Release a single send info
void offload_io_service_impl::_release_send_buff(send_client_info_t& info, frame_buff* buff)
{
    info.inline_io->release_send_buff(frame_buff::uptr(buff));
    assert(info.num_frames_in_use > 0);
    info.num_frames_in_use--;
}

// Flush client queues and unreserve its frames
void offload_io_service_impl::_disconnect_recv_client(recv_client_info_t& info)
{
    auto release_buff = [&info](frame_buff* buff) {
        info.inline_io->release_recv_buff(frame_buff::uptr(buff));
    };

    info.num_frames_in_use -= info.port->offload_thread_flush(release_buff);
    assert(info.num_frames_in_use == 0);
    _reservation_mgr.unreserve_frames(info.frames_reserved);

    // Client waits for a notification after requesting disconnect, so notify it
    info.port->offload_thread_set_connected(false);
}

// Flush client queues and unreserve its frames
void offload_io_service_impl::_disconnect_send_client(send_client_info_t& info)
{
    auto release_buff = [&info](frame_buff* buff) {
        info.inline_io->release_send_buff(frame_buff::uptr(buff));
    };
    info.num_frames_in_use -= info.port->offload_thread_flush(release_buff);
    assert(info.num_frames_in_use == 0);
    _reservation_mgr.unreserve_frames(info.frames_reserved);

    // Client waits for a notification after requesting disconnect, so notify it
    info.port->offload_thread_set_connected(false);
}

template <bool allow_recv, bool allow_send>
void offload_io_service_impl::_do_work_polling()
{
    uhd::set_thread_affinity(_offload_thread_params.cpu_affinity_list);

    client_req_t client_req;

    while (!_stop_offload_thread) {
        if (allow_recv) {
            // Get recv buffers
            for (auto& recv_info : _recv_clients) {
                _get_recv_buff(recv_info, 0);
            }

            // Release recv buffers
            for (auto it = _recv_clients.begin(); it != _recv_clients.end();) {
                frame_buff* buff;
                bool disconnect;
                std::tie(buff, disconnect) = it->port->offload_thread_pop();
                if (buff) {
                    _release_recv_buff(*it, buff);
                } else if (disconnect) {
                    _disconnect_recv_client(*it);
                    it = _recv_clients.erase(it); // increments it
                    continue;
                }
                ++it;
            }
        }

        if (allow_send) {
            // Get send buffers
            for (auto& send_info : _send_clients) {
                _get_send_buff(send_info);
            }

            // Release send buffers
            for (auto it = _send_clients.begin(); it != _send_clients.end();) {
                frame_buff* buff;
                bool disconnect;
                std::tie(buff, disconnect) = it->port->offload_thread_pop();
                if (buff) {
                    _release_send_buff(*it, buff);
                } else if (disconnect) {
                    _disconnect_send_client(*it);
                    it = _send_clients.erase(it); // increments it
                    continue;
                }
                ++it;
            }
        }

        // Execute one client connect command per main loop iteration
        if (_client_connect_queue.pop(client_req)) {
            (*client_req.req)();
            delete client_req.req;
        }
    }
}

template <bool allow_recv, bool allow_send>
void offload_io_service_impl::_do_work_blocking()
{
    uhd::set_thread_affinity(_offload_thread_params.cpu_affinity_list);

    client_req_t client_req;

    while (!_stop_offload_thread) {
        if (allow_recv) {
            // Get recv buffers
            for (auto& recv_info : _recv_clients) {
                _get_recv_buff(recv_info, blocking_timeout_ms);
            }

            // Release recv buffers
            for (auto it = _recv_clients.begin(); it != _recv_clients.end();) {
                frame_buff* buff;
                bool disconnect;

                if (it->num_frames_in_use == it->frames_reserved.num_recv_frames) {
                    // If all buffers are in use, block to avoid excessive CPU usage
                    std::tie(buff, disconnect) = it->port->offload_thread_pop(blocking_timeout_ms);
                } else {
                    // Otherwise, just check current status
                    std::tie(buff, disconnect) = it->port->offload_thread_pop();
                }

                if (buff) {
                    _release_recv_buff(*it, buff);
                } else if (disconnect) {
                    _disconnect_recv_client(*it);
                    it = _recv_clients.erase(it); // increments it
                    continue;
                }
                ++it;
            }
        }

        if (allow_send) {
            // Get send buffers
            for (auto& send_info : _send_clients) {
                _get_send_buff(send_info);
            }

            // Release send buffers
            for (auto it = _send_clients.begin(); it != _send_clients.end();) {
                if (it->num_frames_in_use > 0) {
                    frame_buff* buff;
                    bool disconnect;
                    std::tie(buff, disconnect) = it->port->offload_thread_pop(blocking_timeout_ms);

                    if (buff) {
                        _release_send_buff(*it, buff);
                    } else if (disconnect) {
                        _disconnect_send_client(*it);
                        it = _send_clients.erase(it); // increments it
                        continue;
                    }
                }
                ++it;
            }
        }

        // Execute one client connect command per main loop iteration
        // TODO: In a blocking I/O strategy, the loop can take a long time to
        // service these requests. Need to configure all clients up-front,
        // before starting the offload thread to avoid this.
        if (_client_connect_queue.pop(client_req)) {
            (*client_req.req)();
            delete client_req.req;
        }
    }
}

}} // namespace uhd::transport
