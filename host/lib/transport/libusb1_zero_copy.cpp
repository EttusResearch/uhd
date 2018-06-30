//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "libusb1_base.hpp"
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <list>

#ifdef UHD_TXRX_DEBUG_PRINTS
#include <vector>
#include <fstream>
#include <boost/format.hpp>
#endif

using namespace uhd;
using namespace uhd::transport;

static const size_t DEFAULT_NUM_XFERS = 16;     //num xfers
static const size_t DEFAULT_XFER_SIZE = 32*512; //bytes

//! type for sharing the release queue with managed buffers
class libusb_zero_copy_mb;
typedef boost::shared_ptr<bounded_buffer<libusb_zero_copy_mb *> > mb_queue_sptr;

/*!
 * The libusb docs state that status and actual length can only be read in the callback.
 * Therefore, this struct is intended to store data seen from the callback function.
 */
struct lut_result_t
{
    lut_result_t(void)
    {
        completed = 0;
        status = LIBUSB_TRANSFER_COMPLETED;
        actual_length = 0;
#ifdef UHD_TXRX_DEBUG_PRINTS
        start_time = 0;
        buff_num = -1;
#endif
    }
    int completed;
    libusb_transfer_status status;
    int actual_length;
    boost::mutex mut;
    boost::condition_variable usb_transfer_complete;

#ifdef UHD_TXRX_DEBUG_PRINTS
    // These are fore debugging
    long start_time;
    int buff_num;
    bool is_recv;
#endif
};

// Created to be used as an argument to boost::condition_variable::timed_wait() function
struct lut_result_completed {
    const lut_result_t& _result;
    lut_result_completed(const lut_result_t& result):_result(result) {}
    bool operator()() const {return (_result.completed ? true : false);}
};

#ifdef UHD_TXRX_DEBUG_PRINTS
static std::string dbg_prefix("libusb1_zero_copy,");
static void libusb1_zerocopy_dbg_print_err(std::string msg){
	msg = dbg_prefix + msg;
	fprintf(stderr, "%s\n", msg.c_str());
}
#endif

/*!
 * All libusb callback functions should be marked with the LIBUSB_CALL macro
 * to ensure that they are compiled with the same calling convention as libusb.
 */

//! helper function: handles all async callbacks
static void LIBUSB_CALL libusb_async_cb(libusb_transfer *lut)
{
    lut_result_t *r = (lut_result_t *)lut->user_data;
    boost::lock_guard<boost::mutex> lock(r->mut);
    r->status = lut->status;
    r->actual_length = lut->actual_length;
    r->completed = 1;
    r->usb_transfer_complete.notify_one();  // wake up thread waiting in wait_for_completion() member function below
#ifdef UHD_TXRX_DEBUG_PRINTS
    long end_time = boost::get_system_time().time_of_day().total_microseconds();
    libusb1_zerocopy_dbg_print_err( (boost::format("libusb_async_cb,%s,%i,%i,%i,%ld,%ld") % (r->is_recv ? "rx":"tx") % r->buff_num % r->actual_length % r->status % end_time % r->start_time).str() );
#endif
}

/***********************************************************************
 * Reusable managed buffer:
 *  - Associated with a particular libusb transfer struct.
 *  - Submits the transfer to libusb in the release method.
 **********************************************************************/
class libusb_zero_copy_mb : public managed_buffer
{
public:
    libusb_zero_copy_mb(libusb_transfer *lut, const size_t frame_size, boost::function<void(libusb_zero_copy_mb *)> release_cb, const bool is_recv, const std::string &name):
        _release_cb(release_cb), _is_recv(is_recv), _name(name),
        _ctx(libusb::session::get_global_session()->get_context()),
        _lut(lut), _frame_size(frame_size) { /* NOP */ }

    virtual ~libusb_zero_copy_mb(void);

    void release(void){
    	_release_cb(this);
    }

    UHD_INLINE void submit(void)
    {
        _lut->length = int((_is_recv)? _frame_size : size()); //always set length
#ifdef UHD_TXRX_DEBUG_PRINTS
        result.start_time = boost::get_system_time().time_of_day().total_microseconds();
        result.buff_num = num();
        result.is_recv = _is_recv;
#endif
	int ret = libusb_submit_transfer(_lut);
        if (ret != LIBUSB_SUCCESS)
	  throw uhd::usb_error(ret, str(boost::format(
            "usb %s submit failed: %s") % _name % libusb_error_name(ret)));
    }

    template <typename buffer_type>
    UHD_INLINE typename buffer_type::sptr get_new(const double timeout)
    {
        if (wait_for_completion(timeout))
        {
            if (result.status != LIBUSB_TRANSFER_COMPLETED)
                throw uhd::io_error(str(boost::format("usb %s transfer status: %d")
                                        % _name % libusb_error_name(result.status)));
            result.completed = 0;
            return make(reinterpret_cast<buffer_type *>(this), _lut->buffer, (_is_recv)? size_t(result.actual_length) : _frame_size);
        }
        return typename buffer_type::sptr();
    }

    // This is public because it is accessed from the libusb_zero_copy_single constructor
    lut_result_t result;

    /*!
     * Wait for a managed buffer to become complete.
     *
     * \param timeout the wait timeout in seconds.  A negative value will wait forever.
     * \return true for completion, false for timeout
     */
    UHD_INLINE bool wait_for_completion(const double timeout)
    {
        boost::unique_lock<boost::mutex> lock(result.mut);
        if (!result.completed) {
            if (timeout < 0.0) {
                result.usb_transfer_complete.wait(lock);
            } else {
                const boost::system_time timeout_time = boost::get_system_time() + boost::posix_time::microseconds(long(timeout*1000000));
                result.usb_transfer_complete.timed_wait(lock, timeout_time, lut_result_completed(result));
            }
        }
        return (result.completed > 0);
    }

private:
    boost::function<void(libusb_zero_copy_mb *)> _release_cb;
    const bool _is_recv;
    const std::string _name;
    libusb_context *_ctx;
    libusb_transfer *_lut;
    const size_t _frame_size;
};

libusb_zero_copy_mb::~libusb_zero_copy_mb(void) {
    /* NOP */
}

/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
class libusb_zero_copy_single
{
public:
    libusb_zero_copy_single(
        libusb::device_handle::sptr handle,
        const int interface, const unsigned char endpoint,
        const size_t num_frames, const size_t frame_size
    ):
        _handle(handle),
        _num_frames(num_frames),
        _frame_size(frame_size),
        _buffer_pool(buffer_pool::make(_num_frames, _frame_size)),
        _enqueued(_num_frames), _released(_num_frames),
        _status(STATUS_RUNNING)
    {
        const bool is_recv = (endpoint & 0x80) != 0;
        const std::string name = str(boost::format("%s%d") % ((is_recv)? "rx" : "tx") % int(endpoint & 0x7f));
        _handle->claim_interface(interface);

        //flush the buffers out of the recv endpoint
        //limit the flushing to at most one second
        if (is_recv) for (size_t i = 0; i < 100; i++)
        {
            unsigned char buff[512];
            int transfered = 0;
            const int status = libusb_bulk_transfer(
                _handle->get(), // dev_handle
                endpoint, // endpoint
                static_cast<unsigned char *>(buff),
                int(sizeof(buff)),
                &transfered, //bytes xfered
                10 //timeout ms
            );
            if (status == LIBUSB_ERROR_TIMEOUT) break;
        }

        //allocate libusb transfer structs and managed buffers
        for (size_t i = 0; i < get_num_frames(); i++)
        {
            libusb_transfer *lut = libusb_alloc_transfer(0);
            UHD_ASSERT_THROW(lut != NULL);

            _mb_pool.push_back(boost::make_shared<libusb_zero_copy_mb>(
                lut, this->get_frame_size(), boost::bind(&libusb_zero_copy_single::enqueue_buffer, this, _1), is_recv, name
            ));

            libusb_fill_bulk_transfer(
                lut,                                                    // transfer
                _handle->get(),                                         // dev_handle
                endpoint,                                               // endpoint
                static_cast<unsigned char *>(_buffer_pool->at(i)),      // buffer
                int(this->get_frame_size()),                            // length
                libusb_transfer_cb_fn(&libusb_async_cb),                // callback
                static_cast<void *>(&_mb_pool.back()->result),          // user_data
                0                                                       // timeout (ms)
            );

            _all_luts.push_back(lut);
        }

        //initial release for all buffers
        for (size_t i = 0; i < get_num_frames(); i++)
        {
            libusb_zero_copy_mb &mb = *(_mb_pool[i]);
            if (is_recv) mb.release();
            else
            {
                mb.result.completed = 1;
                _enqueued.push_back(&mb);
            }
        }
    }

    ~libusb_zero_copy_single(void)
    {
        //cancel all transfers
        for(libusb_transfer *lut:  _all_luts)
        {
            libusb_cancel_transfer(lut);
        }

        //process all transfers until timeout occurs
        for(libusb_zero_copy_mb *mb:  _enqueued)
        {
            mb->wait_for_completion(0.01);
        }

        //free all transfers
        for(libusb_transfer *lut:  _all_luts)
        {
            libusb_free_transfer(lut);
        }
    }

    template <typename buffer_type>
    UHD_INLINE typename buffer_type::sptr get_buff(double timeout)
    {
        typename buffer_type::sptr buff;

        if (_status == STATUS_ERROR)
            return buff;

        // Serialize access to buffers
        boost::mutex::scoped_lock get_buff_lock(_get_buff_mutex);

        boost::mutex::scoped_lock queue_lock(_queue_mutex);
        if (_enqueued.empty())
        {
            _buff_ready_cond.timed_wait(queue_lock, boost::posix_time::microseconds(long(timeout*1e6)));
        }
        if (_enqueued.empty()) return buff;
        libusb_zero_copy_mb *front = _enqueued.front();

        queue_lock.unlock();
        buff = front->get_new<buffer_type>(timeout);
        queue_lock.lock();

        if (buff) _enqueued.pop_front();
        this->submit_what_we_can();
        return buff;
    }

    UHD_INLINE size_t get_num_frames(void) const { return _num_frames; }
    UHD_INLINE size_t get_frame_size(void) const { return _frame_size; }

private:
    libusb::device_handle::sptr _handle;
    const size_t _num_frames, _frame_size;

    //! Storage for transfer related objects
    buffer_pool::sptr _buffer_pool;
    std::vector<boost::shared_ptr<libusb_zero_copy_mb> > _mb_pool;

    boost::mutex _queue_mutex;
    boost::condition_variable _buff_ready_cond;
    boost::mutex _get_buff_mutex;

    //! why 2 queues? there is room in the future to have > N buffers but only N in flight
    boost::circular_buffer<libusb_zero_copy_mb *> _enqueued, _released;

    enum {STATUS_RUNNING, STATUS_ERROR} _status;

    void enqueue_buffer(libusb_zero_copy_mb *mb)
    {
        boost::mutex::scoped_lock l(_queue_mutex);
        _released.push_back(mb);
        this->submit_what_we_can();
        _buff_ready_cond.notify_one();
    }

    void submit_what_we_can(void)
    {
        if (_status == STATUS_ERROR)
            return;
        while (not _released.empty() and not _enqueued.full())
        {
            try {
                _released.front()->submit();
                _enqueued.push_back(_released.front());
                _released.pop_front();
            }
            catch (uhd::usb_error& e)
            {
                _status = STATUS_ERROR;
                throw e;
            }
        }
    }

    //! a list of all transfer structs we allocated
    std::list<libusb_transfer *> _all_luts;
};

/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
struct libusb_zero_copy_impl : usb_zero_copy
{
    libusb_zero_copy_impl(
        libusb::device_handle::sptr handle,
        const int recv_interface,
        const unsigned char recv_endpoint,
        const int send_interface,
        const unsigned char send_endpoint,
        const device_addr_t &hints
    ){
        _recv_impl.reset(new libusb_zero_copy_single(
            handle, recv_interface, (recv_endpoint & 0x7f) | 0x80,
            size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_XFERS)),
            size_t(hints.cast<double>("recv_frame_size", DEFAULT_XFER_SIZE))));
        _send_impl.reset(new libusb_zero_copy_single(
            handle, send_interface, (send_endpoint & 0x7f) | 0x00,
            size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_XFERS)),
            size_t(hints.cast<double>("send_frame_size", DEFAULT_XFER_SIZE))));
    }

    virtual ~libusb_zero_copy_impl(void);

    managed_recv_buffer::sptr get_recv_buff(double timeout)
    {
        boost::mutex::scoped_lock l(_recv_mutex);
        return _recv_impl->get_buff<managed_recv_buffer>(timeout);
    }

    managed_send_buffer::sptr get_send_buff(double timeout)
    {
        boost::mutex::scoped_lock l(_send_mutex);
        return _send_impl->get_buff<managed_send_buffer>(timeout);
    }

    size_t get_num_recv_frames(void) const { return _recv_impl->get_num_frames(); }
    size_t get_num_send_frames(void) const { return _send_impl->get_num_frames(); }

    size_t get_recv_frame_size(void) const { return _recv_impl->get_frame_size(); }
    size_t get_send_frame_size(void) const { return _send_impl->get_frame_size(); }

    boost::shared_ptr<libusb_zero_copy_single> _recv_impl, _send_impl;
    boost::mutex _recv_mutex, _send_mutex;
};

libusb_zero_copy_impl::~libusb_zero_copy_impl(void) {
    /* NOP */
}

/***********************************************************************
 * USB zero_copy destructor
 **********************************************************************/
usb_zero_copy::~usb_zero_copy(void) {
    /* NOP */
}

/***********************************************************************
 * USB zero_copy make functions
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make(
    usb_device_handle::sptr handle,
    const int recv_interface,
    const unsigned char recv_endpoint,
    const int send_interface,
    const unsigned char send_endpoint,
    const device_addr_t &hints
){
    libusb::device_handle::sptr dev_handle(libusb::device_handle::get_cached_handle(
        boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
    ));
    return sptr(new libusb_zero_copy_impl(
        dev_handle, recv_interface, recv_endpoint, send_interface, send_endpoint, hints
    ));
}
