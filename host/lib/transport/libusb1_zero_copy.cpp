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
#include <uhd/utils/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;

static const double CLEANUP_TIMEOUT   = 0.2;    //seconds
static const size_t DEFAULT_NUM_XFERS = 16;     //num xfers
static const size_t DEFAULT_XFER_SIZE = 32*512; //bytes

/***********************************************************************
 * Helper functions
 ***********************************************************************/
/*
 * Print the values of a libusb_transfer struct
 * http://libusb.sourceforge.net/api-1.0/structlibusb__transfer.html
 */
void pp_transfer(libusb_transfer *lut)
{
    std::cout << "Libusb transfer"       << std::endl;
    std::cout << "    flags:         0x" << std::hex << (unsigned int) lut->flags << std::endl;
    std::cout << "    endpoint:      0x" << std::hex << (unsigned int) lut->endpoint << std::endl;
    std::cout << "    type:          0x" << std::hex << (unsigned int) lut->type << std::endl;
    std::cout << "    timeout:       "   << std::dec << lut->timeout << std::endl;
    std::cout << "    status:        0x" << std::hex << lut->status << std::endl;
    std::cout << "    length:        "   << std::dec << lut->length << std::endl;
    std::cout << "    actual_length: "   << std::dec << lut->actual_length << std::endl;
}

/***********************************************************************
 * USB asynchronous zero_copy endpoint
 *   This endpoint implementation provides asynchronous I/O to libusb-1.0
 *   devices. Each endpoint is directional and two can be combined to
 *   create a bidirectional interface. It is a zero copy implementation
 *   with respect to libusb, however, each send and recv requires a copy
 *   operation from kernel to userspace; this is due to the usbfs
 *   interface provided by the kernel.
 **********************************************************************/
class usb_endpoint {
public:
    typedef boost::shared_ptr<usb_endpoint> sptr;

    usb_endpoint(
        libusb::device_handle::sptr handle,
        int endpoint,
        bool input,
        size_t transfer_size,
        size_t num_transfers
    );

    ~usb_endpoint(void);

    // Exposed interface for submitting / retrieving transfer buffers

    //! Submit a new transfer that was presumably just filled or emptied.
    void submit(libusb_transfer *lut);

    /*!
     * Get an available transfer:
     * For inputs, this is a just filled transfer.
     * For outputs, this is a just emptied transfer.
     * \param timeout the timeout to wait for a lut
     * \return the transfer pointer or NULL if timeout
     */
    libusb_transfer *get_lut_with_wait(double timeout);

    //Callback use only
    void callback_handle_transfer(libusb_transfer *lut);

private:
    libusb::device_handle::sptr _handle;
    int  _endpoint;
    bool _input;

    //! hold a bounded buffer of completed transfers
    bounded_buffer<libusb_transfer *> _completed_list;

    //! a list of all transfer structs we allocated
    std::vector<libusb_transfer *> _all_luts;

    //! memory allocated for the transfer buffers
    buffer_pool::sptr _buffer_pool;

    // Calls for processing asynchronous I/O
    libusb_transfer *allocate_transfer(void *mem, size_t len);
    void print_transfer_status(libusb_transfer *lut);
};


/*
 * Callback function called when submitted transfers complete.
 * The endpoint upon which the transfer is part of is recovered
 * and the transfer moved from pending to completed state.
 * Callbacks occur during the reaping calls where libusb_handle_events()
 * is used. The callback only modifies the transfer state by moving
 * it from the pending to completed status list.
 * \param lut pointer to libusb_transfer
 */
static void callback(libusb_transfer *lut){
    usb_endpoint *endpoint = (usb_endpoint *) lut->user_data;
    endpoint->callback_handle_transfer(lut);
}


/*
 * Accessor call to allow list access from callback space
 * \param pointer to libusb_transfer
 */
void usb_endpoint::callback_handle_transfer(libusb_transfer *lut){
    _completed_list.push_with_haste(lut);
}


/*
 * Constructor
 * Allocate libusb transfers and mark as free.  For IN endpoints,
 * submit the transfers so that they're ready to return when
 * data is available.
 */
usb_endpoint::usb_endpoint(
    libusb::device_handle::sptr handle,
    int endpoint,
    bool input,
    size_t transfer_size,
    size_t num_transfers
):
    _handle(handle),
    _endpoint(endpoint),
    _input(input),
    _completed_list(num_transfers)
{
    _buffer_pool = buffer_pool::make(num_transfers, transfer_size);
    for (size_t i = 0; i < num_transfers; i++){
        _all_luts.push_back(allocate_transfer(_buffer_pool->at(i), transfer_size));

        //input luts are immediately submitted to be filled
        //output luts go into the completed list as free buffers
        if (_input) this->submit(_all_luts.back());
        else _completed_list.push_with_haste(_all_luts.back());
    }
}


/*
 * Destructor
 * Make sure all the memory is freed. Cancel any pending transfers.
 * When all completed transfers are moved to the free list, release
 * the transfers. Libusb will deallocate the data buffer held by
 * each transfer.
 */
usb_endpoint::~usb_endpoint(void){
    //cancel all transfers
    BOOST_FOREACH(libusb_transfer *lut, _all_luts){
        libusb_cancel_transfer(lut);
    }

    //collect canceled transfers (drain the queue)
    while (this->get_lut_with_wait(CLEANUP_TIMEOUT) != NULL){};

    //free all transfers
    BOOST_FOREACH(libusb_transfer *lut, _all_luts){
        libusb_free_transfer(lut);
    }
}


/*
 * Allocate a libusb transfer
 * The allocated transfer - and buffer it contains - is repeatedly
 * submitted, reaped, and reused and should not be freed until shutdown.
 * \param mem a pointer to the buffer memory
 * \param len size of the individual buffer
 * \return pointer to an allocated libusb_transfer
 */
libusb_transfer *usb_endpoint::allocate_transfer(void *mem, size_t len){
    libusb_transfer *lut = libusb_alloc_transfer(0);
    UHD_ASSERT_THROW(lut != NULL);

    unsigned int endpoint = ((_endpoint & 0x7f) | (_input ? 0x80 : 0));
    unsigned char *buff = reinterpret_cast<unsigned char *>(mem);
    libusb_transfer_cb_fn lut_callback = libusb_transfer_cb_fn(&callback);

    libusb_fill_bulk_transfer(lut,                // transfer
                              _handle->get(),     // dev_handle
                              endpoint,           // endpoint
                              buff,               // buffer
                              len,                // length
                              lut_callback,       // callback
                              this,               // user_data
                              0);                 // timeout
    return lut;
}


/*
 * Asynchonous transfer submission
 * Submit a libusb transfer to libusb add pending status
 * \param lut pointer to libusb_transfer
 * \return true on success or false on error
 */
void usb_endpoint::submit(libusb_transfer *lut){
    UHD_ASSERT_THROW(libusb_submit_transfer(lut) == 0);
}

/*
 * Print status errors of a completed transfer
 * \param lut pointer to an libusb_transfer
 */
void usb_endpoint::print_transfer_status(libusb_transfer *lut){
    std::cout << "here " << lut->status << std::endl;
    switch (lut->status) {
    case LIBUSB_TRANSFER_COMPLETED:
        if (lut->actual_length < lut->length) {
            std::cerr << "USB: transfer completed with short write,"
                      << " length = " << lut->length
                      << " actual = " << lut->actual_length << std::endl;
        }

        if ((lut->actual_length < 0) || (lut->length < 0)) {
            std::cerr << "USB: transfer completed with invalid response"
                      << std::endl;
        }
        break;
    case LIBUSB_TRANSFER_CANCELLED:
        break;
    case LIBUSB_TRANSFER_NO_DEVICE:
        std::cerr << "USB: device was disconnected" << std::endl;
        break;
    case LIBUSB_TRANSFER_OVERFLOW:
        std::cerr << "USB: device sent more data than requested" << std::endl;
        break;
    case LIBUSB_TRANSFER_TIMED_OUT:
        std::cerr << "USB: transfer timed out" << std::endl;
        break;
    case LIBUSB_TRANSFER_STALL:
        std::cerr << "USB: halt condition detected (stalled)" << std::endl;
        break;
    case LIBUSB_TRANSFER_ERROR:
        std::cerr << "USB: transfer failed" << std::endl;
        break;
    default:
        std::cerr << "USB: received unknown transfer status" << std::endl;
    }
}

libusb_transfer *usb_endpoint::get_lut_with_wait(double timeout){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    libusb_transfer *lut = NULL;
    if (_completed_list.pop_with_timed_wait(lut, timeout)) return lut;
    return NULL;
}

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
    );

    ~libusb_zero_copy_impl(void){
        _threads_running = false;
        _thread_group.interrupt_all();
        _thread_group.join_all();
    }

    managed_recv_buffer::sptr get_recv_buff(double);
    managed_send_buffer::sptr get_send_buff(double);

    size_t get_num_recv_frames(void) const { return _num_recv_frames; }
    size_t get_num_send_frames(void) const { return _num_send_frames; }

    size_t get_recv_frame_size(void) const { return _recv_frame_size; }
    size_t get_send_frame_size(void) const { return _send_frame_size; }

private:
    void release(libusb_transfer *lut){
        _recv_ep->submit(lut);
    }

    void commit(libusb_transfer *lut, size_t num_bytes){
        lut->length = num_bytes;
        try{
            _send_ep->submit(lut);
        }
        catch(const std::exception &e){
            std::cerr << "Error in commit: " << e.what() << std::endl;
        }
    }

    libusb::device_handle::sptr _handle;
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    usb_endpoint::sptr _recv_ep, _send_ep;

    //event handler threads
    boost::thread_group _thread_group;
    bool _threads_running;

    void run_event_loop(void){
        set_thread_priority_safe();
        libusb::session::sptr session = libusb::session::get_global_session();
        _threads_running = true;
        try{
            while(_threads_running){
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000; //100ms
                libusb_handle_events_timeout(session->get_context(), &tv);
            }
        } catch(const boost::thread_interrupted &){}
    }
};

/*
 * Constructor
 * Initializes libusb, opens devices, and sets up interfaces for I/O.
 * Finally, creates endpoints for asynchronous I/O.
 */
libusb_zero_copy_impl::libusb_zero_copy_impl(
    libusb::device_handle::sptr handle,
    size_t recv_endpoint,
    size_t send_endpoint,
    const device_addr_t &hints
):
    _handle(handle),
    _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", DEFAULT_XFER_SIZE))),
    _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_XFERS))),
    _send_frame_size(size_t(hints.cast<double>("send_frame_size", DEFAULT_XFER_SIZE))),
    _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_XFERS)))
{
    _handle->claim_interface(2 /*in interface*/);
    _handle->claim_interface(1 /*out interface*/);

    _recv_ep = usb_endpoint::sptr(new usb_endpoint(
                              _handle,         // libusb device_handle
                              recv_endpoint,   // USB endpoint number
                              true,            // IN endpoint
                              this->get_recv_frame_size(),  // buffer size per transfer
                              this->get_num_recv_frames()   // number of libusb transfers
    ));

    _send_ep = usb_endpoint::sptr(new usb_endpoint(
                              _handle,         // libusb device_handle
                              send_endpoint,   // USB endpoint number
                              false,           // OUT endpoint
                              this->get_send_frame_size(),  // buffer size per transfer
                              this->get_num_send_frames()   // number of libusb transfers
    ));

    //spawn the event handler threads
    size_t concurrency = hints.cast<size_t>("concurrency_hint", 1);
    for (size_t i = 0; i < concurrency; i++) _thread_group.create_thread(
        boost::bind(&libusb_zero_copy_impl::run_event_loop, this)
    );
}

/*
 * Construct a managed receive buffer from a completed libusb transfer
 * (happy with buffer full of data) obtained from the receive endpoint.
 * Return empty pointer if no transfer is available (timeout or error).
 * \return pointer to a managed receive buffer
 */
managed_recv_buffer::sptr libusb_zero_copy_impl::get_recv_buff(double timeout){
    libusb_transfer *lut = _recv_ep->get_lut_with_wait(timeout);
    if (lut == NULL) {
        return managed_recv_buffer::sptr();
    }
    else {
        return managed_recv_buffer::make_safe(
            lut->buffer, lut->actual_length,
            boost::bind(&libusb_zero_copy_impl::release, this, lut)
        );
    }
}


/*
 * Construct a managed send buffer from a free libusb transfer (with
 * empty buffer). Return empty pointer of no transfer is available
 * (timeout or error).
 * \return pointer to a managed send buffer
 */
managed_send_buffer::sptr libusb_zero_copy_impl::get_send_buff(double timeout){
    libusb_transfer *lut = _send_ep->get_lut_with_wait(timeout);
    if (lut == NULL) {
        return managed_send_buffer::sptr();
    }
    else {
        return managed_send_buffer::make_safe(
            lut->buffer, this->get_send_frame_size(),
            boost::bind(&libusb_zero_copy_impl::commit, this, lut, _1)
        );
    }
}

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
