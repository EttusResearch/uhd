//
// Copyright 2010-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "libusb1_base.hpp"
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <list>

using namespace uhd;
using namespace uhd::transport;

static const size_t DEFAULT_NUM_XFERS = 16;     //num xfers
static const size_t DEFAULT_XFER_SIZE = 32*512; //bytes

//! Define LIBUSB_CALL when its missing (non-windows)
#ifndef LIBUSB_CALL
    #define LIBUSB_CALL
#endif /*LIBUSB_CALL*/

/*!
 * All libusb callback functions should be marked with the LIBUSB_CALL macro
 * to ensure that they are compiled with the same calling convention as libusb.
 */

//! helper function: handles all async callbacks
static void LIBUSB_CALL libusb_async_cb(libusb_transfer *lut){
    *(static_cast<bool *>(lut->user_data)) = true;
}

/*!
 * Wait for a managed buffer to become complete.
 *
 * This routine processes async events until the transaction completes.
 * We must call the libusb handle events in a loop because the handler
 * may complete managed buffers other than the one we are waiting on.
 *
 * We cannot determine if handle events timed out or processed an event.
 * Therefore, the timeout condition is handled by using boost system time.
 *
 * \param ctx the libusb context structure
 * \param timeout the wait timeout in seconds
 * \param completed a reference to the completed flag
 * \return true for completion, false for timeout
 */
UHD_INLINE bool wait_for_completion(libusb_context *ctx, const double timeout, bool &completed){
    //already completed by a previous call?
    if (completed) return true;

    //perform a non-blocking event handle
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    libusb_handle_events_timeout(ctx, &tv);
    if (completed) return true;

    //finish the rest with a timeout loop
    const boost::system_time timeout_time = boost::get_system_time() + boost::posix_time::microseconds(long(timeout*1000000));
    while (not completed and (boost::get_system_time() < timeout_time)){
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000; /*10ms*/
        libusb_handle_events_timeout(ctx, &tv);
    }

    return completed;
}

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - Associated with a particular libusb transfer struct.
 *  - Submits the transfer to libusb in the release method.
 **********************************************************************/
class libusb_zero_copy_mrb : public managed_recv_buffer{
public:
    libusb_zero_copy_mrb(libusb_transfer *lut, const size_t frame_size):
        _ctx(libusb::session::get_global_session()->get_context()),
        _lut(lut), _frame_size(frame_size) { /* NOP */ }

    void release(void){
        completed = false;
        _lut->length = _frame_size; //always reset length
        UHD_ASSERT_THROW(libusb_submit_transfer(_lut) == 0);
    }

    sptr get_new(const double timeout, size_t &index){
        if (wait_for_completion(_ctx, timeout, completed)){
            index++;
            return make(this, _lut->buffer, _lut->actual_length);
        }
        return managed_recv_buffer::sptr();
    }

    bool completed;

private:
    libusb_context *_ctx;
    libusb_transfer *_lut;
    const size_t _frame_size;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - Associated with a particular libusb transfer struct.
 *  - Submits the transfer to libusb in the commit method.
 **********************************************************************/
class libusb_zero_copy_msb : public managed_send_buffer{
public:
    libusb_zero_copy_msb(libusb_transfer *lut, const size_t frame_size):
        _ctx(libusb::session::get_global_session()->get_context()),
        _lut(lut), _frame_size(frame_size) { completed = true; }

    void release(void){
        completed = false;
        _lut->length = size();
        UHD_ASSERT_THROW(libusb_submit_transfer(_lut) == 0);
    }

    sptr get_new(const double timeout, size_t &index){
        if (wait_for_completion(_ctx, timeout, completed)){
            index++;
            return make(this, _lut->buffer, _frame_size);
        }
        return managed_send_buffer::sptr();
    }

    bool completed;

private:
    libusb_context *_ctx;
    libusb_transfer *_lut;
    const size_t _frame_size;
};

/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
class libusb_zero_copy_impl : public usb_zero_copy{
public:

    libusb_zero_copy_impl(
        libusb::device_handle::sptr handle,
        const size_t recv_interface,
        const size_t recv_endpoint,
        const size_t send_interface,
        const size_t send_endpoint,
        const device_addr_t &hints
    ):
        _handle(handle),
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", DEFAULT_XFER_SIZE))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_XFERS))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", DEFAULT_XFER_SIZE))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_XFERS))),
        _recv_buffer_pool(buffer_pool::make(_num_recv_frames, _recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(_num_send_frames, _send_frame_size)),
        _next_recv_buff_index(0),
        _next_send_buff_index(0)
    {
        _handle->claim_interface(recv_interface);
        _handle->claim_interface(send_interface);

        //flush the buffers out of the recv endpoint
        //limit the flushing to at most one second
        for (size_t i = 0; i < 100; i++)
        {
            unsigned char buff[512];
            int transfered = 0;
            const int status = libusb_bulk_transfer(
                _handle->get(), // dev_handle
                (recv_endpoint & 0x7f) | 0x80, // endpoint
                static_cast<unsigned char *>(buff),
                sizeof(buff),
                &transfered, //bytes xfered
                10 //timeout ms
            );
            if (status == LIBUSB_ERROR_TIMEOUT) break;
        }

        //allocate libusb transfer structs and managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){

            libusb_transfer *lut = libusb_alloc_transfer(0);
            UHD_ASSERT_THROW(lut != NULL);

            _mrb_pool.push_back(boost::make_shared<libusb_zero_copy_mrb>(lut, this->get_recv_frame_size()));

            libusb_fill_bulk_transfer(
                lut,                                                    // transfer
                _handle->get(),                                         // dev_handle
                (recv_endpoint & 0x7f) | 0x80,                          // endpoint
                static_cast<unsigned char *>(_recv_buffer_pool->at(i)), // buffer
                this->get_recv_frame_size(),                            // length
                libusb_transfer_cb_fn(&libusb_async_cb),                // callback
                static_cast<void *>(&_mrb_pool.back()->completed),      // user_data
                0                                                       // timeout (ms)
            );

            _all_luts.push_back(lut);
            _mrb_pool.back()->release();
        }

        //allocate libusb transfer structs and managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){

            libusb_transfer *lut = libusb_alloc_transfer(0);
            UHD_ASSERT_THROW(lut != NULL);

            _msb_pool.push_back(boost::make_shared<libusb_zero_copy_msb>(lut, this->get_send_frame_size()));

            libusb_fill_bulk_transfer(
                lut,                                                    // transfer
                _handle->get(),                                         // dev_handle
                (send_endpoint & 0x7f) | 0x00,                          // endpoint
                static_cast<unsigned char *>(_send_buffer_pool->at(i)), // buffer
                this->get_send_frame_size(),                            // length
                libusb_transfer_cb_fn(&libusb_async_cb),                // callback
                static_cast<void *>(&_msb_pool.back()->completed),      // user_data
                0                                                       // timeout
            );

            _all_luts.push_back(lut);
        }
    }

    ~libusb_zero_copy_impl(void){
        libusb_context *ctx = libusb::session::get_global_session()->get_context();

        //cancel all transfers
        BOOST_FOREACH(libusb_transfer *lut, _all_luts){
            libusb_cancel_transfer(lut);
        }

        //process all transfers until timeout occurs
        bool completed = false;
        wait_for_completion(ctx, 0.01, completed);

        //free all transfers
        BOOST_FOREACH(libusb_transfer *lut, _all_luts){
            libusb_free_transfer(lut);
        }

    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        if (_next_recv_buff_index == _num_recv_frames) _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(timeout, _next_recv_buff_index);
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        if (_next_send_buff_index == _num_send_frames) _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(timeout, _next_send_buff_index);
    }

    size_t get_num_recv_frames(void) const { return _num_recv_frames; }
    size_t get_num_send_frames(void) const { return _num_send_frames; }

    size_t get_recv_frame_size(void) const { return _recv_frame_size; }
    size_t get_send_frame_size(void) const { return _send_frame_size; }

private:
    libusb::device_handle::sptr _handle;
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;

    //! Storage for transfer related objects
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<boost::shared_ptr<libusb_zero_copy_mrb> > _mrb_pool;
    std::vector<boost::shared_ptr<libusb_zero_copy_msb> > _msb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;

    //! a list of all transfer structs we allocated
    std::list<libusb_transfer *> _all_luts;


};

/***********************************************************************
 * USB zero_copy make functions
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make(
    usb_device_handle::sptr handle,
    const size_t recv_interface,
    const size_t recv_endpoint,
    const size_t send_interface,
    const size_t send_endpoint,
    const device_addr_t &hints
){
    libusb::device_handle::sptr dev_handle(libusb::device_handle::get_cached_handle(
        boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
    ));
    return sptr(new libusb_zero_copy_impl(
        dev_handle, recv_interface, recv_endpoint, send_interface, send_endpoint, hints
    ));
}
