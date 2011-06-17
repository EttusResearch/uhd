//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
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

//! helper function: handles all rx async callbacks
static void LIBUSB_CALL libusb_async_rx_cb(libusb_transfer *lut){
    if(lut->actual_length == 0) {
        UHD_ASSERT_THROW(libusb_submit_transfer(lut) == 0); //get out until you find some real data
        return;
    }
    (*static_cast<boost::function<void()> *>(lut->user_data))();
}

//! helper function: handles all tx async callbacks
static void LIBUSB_CALL libusb_async_tx_cb(libusb_transfer *lut) {
    (*static_cast<boost::function<void()> *>(lut->user_data))();
}

//! callback to free transfer upon cancellation
static void LIBUSB_CALL cancel_transfer_cb(libusb_transfer *lut){
    if (lut->status == LIBUSB_TRANSFER_CANCELLED || lut->status == LIBUSB_TRANSFER_TIMED_OUT) libusb_free_transfer(lut);
    else UHD_MSG(error) << "libusb cancel_transfer unexpected status " << lut->status << std::endl;
}

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - Associated with a particular libusb transfer struct.
 *  - Submits the transfer to libusb in the release method.
 **********************************************************************/
class libusb_zero_copy_mrb : public managed_recv_buffer{
public:
    libusb_zero_copy_mrb(libusb_transfer *lut):
        _lut(lut), _expired(true) { /* NOP */ }

    void release(void){
        if (_expired) return;
        UHD_ASSERT_THROW(libusb_submit_transfer(_lut) == 0);
        _expired = true;
    }

    sptr get_new(void){
        _expired = false;
        return make_managed_buffer(this);
    }

private:
    const void *get_buff(void) const{return _lut->buffer;}
    size_t get_size(void) const{return _lut->actual_length;}

    libusb_transfer *_lut;
    bool _expired;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - Associated with a particular libusb transfer struct.
 *  - Submits the transfer to libusb in the commit method.
 **********************************************************************/
class libusb_zero_copy_msb : public managed_send_buffer{
public:
    libusb_zero_copy_msb(libusb_transfer *lut):
        _lut(lut), _expired(true) { /* NOP */ }

    void commit(size_t len){
        if (_expired) return;
        _lut->length = len;
        if(len == 0) libusb_async_tx_cb(_lut);
        else UHD_ASSERT_THROW(libusb_submit_transfer(_lut) == 0);
        _expired = true;
    }

    sptr get_new(void){
        _expired = false;
        return make_managed_buffer(this);
    }

private:
    void *get_buff(void) const{return _lut->buffer;}
    size_t get_size(void) const{return _lut->length;}

    libusb_transfer *_lut;
    bool _expired;
};

/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
class libusb_zero_copy_impl : public usb_zero_copy{
public:

    libusb_zero_copy_impl(
        libusb::device_handle::sptr handle,
        size_t recv_endpoint,
        size_t send_endpoint,
        const device_addr_t &hints
    ):
        _handle(handle),
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", DEFAULT_XFER_SIZE))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_XFERS))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", DEFAULT_XFER_SIZE))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_XFERS))),
        _recv_buffer_pool(buffer_pool::make(_num_recv_frames, _recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(_num_send_frames, _send_frame_size)),
        _pending_recv_buffs(_num_recv_frames),
        _pending_send_buffs(_num_send_frames)
    {
        _handle->claim_interface(2 /*in interface*/);
        _handle->claim_interface(1 /*out interface*/);

        //allocate libusb transfer structs and managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){

            libusb_transfer *lut = libusb_alloc_transfer(0);
            UHD_ASSERT_THROW(lut != NULL);

            _mrb_pool.push_back(libusb_zero_copy_mrb(lut));
            _callbacks.push_back(boost::bind(
                &libusb_zero_copy_impl::handle_recv, this, &_mrb_pool.back()
            ));

            libusb_fill_bulk_transfer(
                lut,                                                    // transfer
                _handle->get(),                                         // dev_handle
                (recv_endpoint & 0x7f) | 0x80,                          // endpoint
                static_cast<unsigned char *>(_recv_buffer_pool->at(i)), // buffer
                this->get_recv_frame_size(),                            // length
                libusb_transfer_cb_fn(&libusb_async_rx_cb),             // callback
                static_cast<void *>(&_callbacks.back()),                // user_data
                0                                                       // timeout (ms)
            );

            _all_luts.push_back(lut);
            _mrb_pool.back().get_new();
        }

        //allocate libusb transfer structs and managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){

            libusb_transfer *lut = libusb_alloc_transfer(0);
            UHD_ASSERT_THROW(lut != NULL);

            _msb_pool.push_back(libusb_zero_copy_msb(lut));
            _callbacks.push_back(boost::bind(
                &libusb_zero_copy_impl::handle_send, this, &_msb_pool.back()
            ));

            libusb_fill_bulk_transfer(
                lut,                                                    // transfer
                _handle->get(),                                         // dev_handle
                (send_endpoint & 0x7f) | 0x00,                          // endpoint
                static_cast<unsigned char *>(_send_buffer_pool->at(i)), // buffer
                this->get_send_frame_size(),                            // length
                libusb_transfer_cb_fn(&libusb_async_tx_cb),             // callback
                static_cast<void *>(&_callbacks.back()),                // user_data
                0                                                       // timeout
            );

            _all_luts.push_back(lut);
            libusb_async_tx_cb(lut);
        }

        //spawn the event handler threads
        size_t concurrency = hints.cast<size_t>("concurrency_hint", 1);
        boost::barrier spawn_barrier(concurrency+1);
        for (size_t i = 0; i < concurrency; i++) _thread_group.create_thread(
            boost::bind(&libusb_zero_copy_impl::run_event_loop, this, boost::ref(spawn_barrier))
        );
        spawn_barrier.wait();
    }

    ~libusb_zero_copy_impl(void){
        //cancel and free all transfers
        BOOST_FOREACH(libusb_transfer *lut, _all_luts){
            lut->callback = libusb_transfer_cb_fn(&cancel_transfer_cb);
            libusb_cancel_transfer(lut);
            while(lut->status != LIBUSB_TRANSFER_CANCELLED
               && lut->status != LIBUSB_TRANSFER_COMPLETED
               && lut->status != LIBUSB_TRANSFER_TIMED_OUT) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            }
        }
        //shutdown the threads
        _thread_group.interrupt_all();
        _thread_group.join_all();
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        libusb_zero_copy_mrb *mrb = NULL;
        if (_pending_recv_buffs.pop_with_timed_wait(mrb, timeout)){
            return mrb->get_new();
        }
        return managed_recv_buffer::sptr();
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        libusb_zero_copy_msb *msb = NULL;
        if (_pending_send_buffs.pop_with_timed_wait(msb, timeout)){
            return msb->get_new();
        }
        return managed_send_buffer::sptr();
    }

    size_t get_num_recv_frames(void) const { return _num_recv_frames; }
    size_t get_num_send_frames(void) const { return _num_send_frames; }

    size_t get_recv_frame_size(void) const { return _recv_frame_size; }
    size_t get_send_frame_size(void) const { return _send_frame_size; }

private:
    //! Handle a bound async callback for recv
    void handle_recv(libusb_zero_copy_mrb *mrb){
        _pending_recv_buffs.push_with_haste(mrb);
    }

    //! Handle a bound async callback for send
    void handle_send(libusb_zero_copy_msb *msb){
        _pending_send_buffs.push_with_haste(msb);
    }

    libusb::device_handle::sptr _handle;
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;

    //! Storage for transfer related objects
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    bounded_buffer<libusb_zero_copy_mrb *> _pending_recv_buffs;
    bounded_buffer<libusb_zero_copy_msb *> _pending_send_buffs;
    std::list<libusb_zero_copy_mrb> _mrb_pool;
    std::list<libusb_zero_copy_msb> _msb_pool;
    std::list<boost::function<void()> > _callbacks;

    //! a list of all transfer structs we allocated
    std::list<libusb_transfer *> _all_luts;

    //! event handler threads
    boost::thread_group _thread_group;

    void run_event_loop(boost::barrier &spawn_barrier){
        spawn_barrier.wait();
        set_thread_priority_safe();
        libusb_context *context = libusb::session::get_global_session()->get_context();
        try{
            while (not boost::this_thread::interruption_requested()){
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000; //100ms
                libusb_handle_events_timeout(context, &tv);
            }
        } catch(const boost::thread_interrupted &){}
    }

};

/***********************************************************************
 * USB zero_copy make functions
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make(
    usb_device_handle::sptr handle,
    size_t recv_endpoint,
    size_t send_endpoint,
    const device_addr_t &hints
){
    libusb::device_handle::sptr dev_handle(libusb::device_handle::get_cached_handle(
        boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
    ));
    return sptr(new libusb_zero_copy_impl(
        dev_handle, recv_endpoint, send_endpoint, hints
    ));
}
