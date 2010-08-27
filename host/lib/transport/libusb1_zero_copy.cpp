//
// Copyright 2010 Ettus Research LLC
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
#include <uhd/utils/assert.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <iomanip>

using namespace uhd::transport;

const int libusb_debug_level = 0;
const int libusb_timeout = 0;

/***********************************************************************
 * Helper functions
 *
 * Print to stdout the values of a libusb_transfer struct
 ***********************************************************************/
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
 * USB asynchronous phony zero_copy endpoint
 *   This endpoint implementation provides asynchronous I/O to libusb-1.0
 *   devices. Each endpoint is directional and two can be combined to
 *   create a bidirectional interface. It is a zero copy implementation
 *   with respect to libusb, however, each send and recv requires a copy
 *   operation from kernel to userspace; this is due to the usbfs
 *   interface provided by the kernel. 
 **********************************************************************/
class usb_endpoint {
private:
    libusb_device_handle *_dev_handle;
    libusb_context *_ctx;
    int  _endpoint;
    bool _input;

    size_t _transfer_size;
    size_t _num_transfers;

    /*
     * Transfer state lists (free, pending, or completed)
     */
    std::list<libusb_transfer *>  _free_list;
    std::list<libusb_transfer *>  _pending_list;
    std::list<libusb_transfer *>  _completed_list;

    /*
     * Calls for processing asynchronous I/O 
     */
    libusb_transfer *allocate_transfer(int buff_len);
    bool cancel(libusb_transfer *lut);
    bool cancel_all();
    bool reap_pending_list();
    bool reap_pending_list_timeout();
    bool reap_completed_list();

    /*
     * Transfer state manipulators 
     */
    void free_list_add(libusb_transfer *lut);
    void pending_list_add(libusb_transfer *lut);
    void completed_list_add(libusb_transfer *lut);
    libusb_transfer *free_list_get();
    libusb_transfer *completed_list_get();
    bool pending_list_remove(libusb_transfer *lut);

    /*
     * Misc
     */
    void print_transfer_status(libusb_transfer *lut);

public:
    usb_endpoint(libusb_device_handle *dev_handle,
                 libusb_context *ctx, int endpoint, bool input,
                 size_t transfer_size, size_t num_transfers);

    ~usb_endpoint();

    /*
     * Accessors 
     */
    int get_endpoint() const { return _endpoint; }
    bool get_direction() const { return _input; }
    libusb_device_handle *get_dev_handle() const { return _dev_handle; }
    libusb_context *get_ctx() const { return _ctx; }

    /*
     * Exposed interface for submitting / retrieving transfer buffers
     * used in zero-copy interface
     */
    bool submit(libusb_transfer *lut);
    libusb_transfer *get_completed_transfer();
    libusb_transfer *get_free_transfer();

    /*
     * Callback use only
     */
    void callback_handle_transfer(libusb_transfer *lut);
};


/*
 * Callback function called when submitted transfers complete.
 * The endpoint upon which the transfer is part of is recovered
 * and the transfer moved from pending to completed state.
 */
static void callback(libusb_transfer *lut)
{
    usb_endpoint *endpoint = (usb_endpoint *) lut->user_data; 
    endpoint->callback_handle_transfer(lut);
}


/*
 * Accessor call to allow list access from callback space
 */
void usb_endpoint::callback_handle_transfer(libusb_transfer *lut)
{
    if (!pending_list_remove(lut)) {
        std::cerr << "USB: pending remove failed" << std::endl;
        return;
    }

    completed_list_add(lut);    
}


/*
 * Constructor
 *
 * Allocate libusb transfers. For IN endpoints, submit the transfers
 * so that they're ready to return when data is available. 
 */
usb_endpoint::usb_endpoint(libusb_device_handle *dev_handle,
                          libusb_context *ctx, int endpoint, bool input,
                          size_t transfer_size, size_t num_transfers)
    : _dev_handle(dev_handle),
      _ctx(ctx), _endpoint(endpoint), _input(input),
      _transfer_size(transfer_size), _num_transfers(num_transfers)
{
    unsigned int i;
    for (i = 0; i < _num_transfers; i++) {
        free_list_add(allocate_transfer(_transfer_size));

        if (_input)
            submit(free_list_get());
    }
}


/*
 * Destructor
 */
usb_endpoint::~usb_endpoint()
{
    cancel_all();

    while (!_pending_list.empty()) {
        if (!reap_pending_list())
            std::cerr << "error: destructor failed to reap" << std::endl;
    }

    while (!_completed_list.empty()) {
        if (!reap_completed_list())
            std::cerr << "error: destructor failed to reap" << std::endl;
    }

    while (!_free_list.empty()) {
        libusb_free_transfer(free_list_get());
    }
}


/*
 * Allocate a libusb transfer 
 *
 * The allocated transfer is continuously reused and should be freed at
 * shutdown.
 */
libusb_transfer *usb_endpoint::allocate_transfer(int buff_len)
{
    libusb_transfer *lut = libusb_alloc_transfer(0);

    unsigned char *buff = new unsigned char[buff_len];

    unsigned int endpoint = ((_endpoint & 0x7f) | (_input ? 0x80 : 0));

    libusb_fill_bulk_transfer(lut,                // transfer
                              _dev_handle,        // dev_handle
                              endpoint,           // endpoint
                              buff,               // buffer
                              buff_len,           // length
                              callback,           // callback
                              this,               // user_data
                              0);                 // timeout
    return lut;
}


/*
 * Asynchonous transfer submission
 *
 * Submit and mark transfer as pending.
 */
bool usb_endpoint::submit(libusb_transfer *lut)
{
    int retval;
    if ((retval = libusb_submit_transfer(lut)) < 0) {
        std::cerr << "error: libusb_submit_transfer: " << retval << std::endl;
        return false;
    }

    pending_list_add(lut);
    return true;
}


/*
 * Cancel a pending transfer 
 *
 * Search the pending list for the transfer and cancel if found.
 * Returns true on success. False otherwise or on error. 
 *
 * Note: success only indicates submission of cancelation request.
 * Sucessful cancelation is not known until the callback occurs.
 */
bool usb_endpoint::cancel(libusb_transfer *lut)
{
    std::list<libusb_transfer*>::iterator iter;
    for (iter = _pending_list.begin(); iter != _pending_list.end(); iter++) {
        if (*iter == lut) { 
            libusb_cancel_transfer(lut); 
            return true;
        }
    }
    return false;
}


/*
 * Cancel all pending transfers 
 *
 * Note: success only indicates submission of cancelation request.
 * Sucessful cancelation is not known until the callback occurs.
 */
bool usb_endpoint::cancel_all()
{
    std::list<libusb_transfer*>::iterator iter;

    for (iter = _pending_list.begin(); iter != _pending_list.end(); iter++) {
        if (libusb_cancel_transfer(*iter) < 0) {
            std::cerr << "error: libusb_cancal_transfer() failed" << std::endl;
            return false;
        }
    }

    return true;
}


/*
 * Reap completed transfers
 * 
 * return true if at least one transfer was reaped, false otherwise. 
 *
 * Check completed transfers for errors and mark as free. This is a
 * blocking call. 
 */
bool usb_endpoint::reap_completed_list()
{
    libusb_transfer *lut;

    if (_completed_list.empty()) {
        if (!reap_pending_list_timeout())
            return false;
    }

    while (!_completed_list.empty()) {
        lut = completed_list_get();
        print_transfer_status(lut);
        free_list_add(lut);
    }

    return true;
}


/*
 * Print completed transfer status error(s) 
 * 
 * return true if at least one transfer was reaped, false otherwise. 
 *
 * Check completed transfers for errors and mark as free. This is a
 * blocking call. 
 */
void usb_endpoint::print_transfer_status(libusb_transfer *lut)
{
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


/*
 * Reap pending transfers 
 *
 * Return true if at least one transfer was reaped, false otherwise. This is
 * a blocking call.
 *
 * Reaping submitted transfers is handled by libusb and the assigned callback
 * function. Block until at least one transfer is reaped.
 */
bool usb_endpoint::reap_pending_list()
{
    int retval;

    if ((retval = libusb_handle_events(_ctx)) < 0) {
        std::cerr << "error: libusb_handle_events: " << retval << std::endl;
        return false;
    }

    return true;
}


/*
 * Reap pending transfers with timeout 
 *
 * Return true if at least one transfer was reaped, false otherwise. This call
 * blocks until a transfer is reaped or timeout.
 *
 * Reaping submitted transfers is handled by libusb and the assigned callback
 * function. Block until at least one transfer is reaped or timeout occurs.
 */
bool usb_endpoint::reap_pending_list_timeout()
{
    int retval;
    timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 100000; //100ms

    size_t pending_list_size = _pending_list.size();

    if ((retval = libusb_handle_events_timeout(_ctx, &tv)) < 0) {
        std::cerr << "error: libusb_handle_events: " << retval << std::endl;
        return false;
    }

    if (_pending_list.size() < pending_list_size) {
        return true;
    }
    else {
        return false;
    }
}


/*
 * Returns a free transfer with empty data bufer for OUT requests 
 */
libusb_transfer *usb_endpoint::get_free_transfer()
{
    if (_free_list.empty()) {
        if (!reap_completed_list())
            return NULL; 
    }

    return free_list_get();
}


/*
 * Returns a transfer containing data for IN requests
 */
libusb_transfer *usb_endpoint::get_completed_transfer()
{
    if (_completed_list.empty()) {
        if (!reap_pending_list_timeout())
            return NULL; 
    }

    return completed_list_get();
}

/*
 * List operations 
 */
void usb_endpoint::free_list_add(libusb_transfer *lut)
{
    _free_list.push_back(lut);
}

void usb_endpoint::pending_list_add(libusb_transfer *lut)
{
    _pending_list.push_back(lut);
}

void usb_endpoint::completed_list_add(libusb_transfer *lut)
{
    _completed_list.push_back(lut);
}


/*
 * Free and completed lists don't have ordered content 
 *
 * Pop transfers from the front as needed
 */
libusb_transfer *usb_endpoint::free_list_get()
{
    libusb_transfer *lut;

    if (_free_list.size() == 0) {
        return NULL; 
    }
    else { 
        lut = _free_list.front();
        _free_list.pop_front();
        return lut;
    }
}


/*
 * Free and completed lists don't have ordered content 
 *
 * Pop transfers from the front as needed
 */
libusb_transfer *usb_endpoint::completed_list_get()
{
    libusb_transfer *lut;

    if (_completed_list.empty()) {
        return NULL;
    }
    else { 
        lut = _completed_list.front();
        _completed_list.pop_front();
        return lut;
    }
}


/*
 * Search and remove transfer from pending list
 *
 * Assuming that the callbacks occur in order, the front element
 * should yield the correct transfer. If not, then something else
 * is going on. If no transfers match, then something went wrong.
 */
bool usb_endpoint::pending_list_remove(libusb_transfer *lut)
{
    std::list<libusb_transfer*>::iterator iter;
    for (iter = _pending_list.begin(); iter != _pending_list.end(); iter++) {
        if (*iter == lut) { 
            _pending_list.erase(iter);
            return true;
        }
    }
    return false;
}


/***********************************************************************
 * Managed buffers 
 **********************************************************************/
class libusb_managed_recv_buffer_impl : public managed_recv_buffer {
public:
    libusb_managed_recv_buffer_impl(libusb_transfer *lut,
                                    usb_endpoint *endpoint)
        : _buff(lut->buffer, lut->length)
    {
        _lut = lut;
        _endpoint = endpoint;
    }

    ~libusb_managed_recv_buffer_impl()
    {
       if (!_endpoint->submit(_lut))
           std::cerr << "USB: failed to submit IN transfer" << std::endl;
    }

private:
    const boost::asio::const_buffer &get() const
    {
        return _buff; 
    }

    libusb_transfer *_lut;
    usb_endpoint *_endpoint;
    const boost::asio::const_buffer _buff;
};


class libusb_managed_send_buffer_impl : public managed_send_buffer {
public:
    libusb_managed_send_buffer_impl(libusb_transfer *lut,
                                    usb_endpoint *endpoint,
                                    size_t buff_size)
        : _buff(lut->buffer, buff_size), _committed(false)
    {
        _lut = lut;
        _endpoint = endpoint;
    }

    ~libusb_managed_send_buffer_impl()
    {
        if (!_committed) {
            _lut->length = 0;
            _lut->actual_length = 0;
            _endpoint->submit(_lut);
        }
    }

    ssize_t commit(size_t num_bytes)
    {
        if (_committed) {
            std::cerr << "UHD: send buffer already committed" << std::endl;
            return 0;
        }
        
        UHD_ASSERT_THROW(num_bytes <= boost::asio::buffer_size(_buff));

        _lut->length = num_bytes;
        _lut->actual_length = 0;

        if (_endpoint->submit(_lut)) {
            _committed = true;
            return num_bytes;
        }
        else {
            return 0;
        }
    }

private:
    const boost::asio::mutable_buffer &get() const
    {
        return _buff; 
    }

    libusb_transfer *_lut;
    usb_endpoint *_endpoint;
    const boost::asio::mutable_buffer _buff;
    bool _committed;
};


/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
class libusb_zero_copy_impl : public usb_zero_copy
{
private:
    usb_endpoint          *_rx_ep;
    usb_endpoint          *_tx_ep;

    /*
     * Libusb handles
     */
    libusb_context       *_rx_ctx;
    libusb_context       *_tx_ctx;
    libusb_device_handle *_rx_dev_handle;
    libusb_device_handle *_tx_dev_handle;

    size_t _recv_buff_size;
    size_t _send_buff_size;
    size_t _num_frames;

public:
    typedef boost::shared_ptr<libusb_zero_copy_impl> sptr;

    /*
     * Structors
     */
    libusb_zero_copy_impl(usb_device_handle::sptr handle,
                          unsigned int rx_endpoint,
                          unsigned int tx_endpoint,
                          size_t recv_buff_size,
                          size_t send_buff_size);
    
    ~libusb_zero_copy_impl();

    managed_recv_buffer::sptr get_recv_buff(void);
    managed_send_buffer::sptr get_send_buff(void);

    size_t get_num_recv_frames(void) const { return _num_frames; }
    size_t get_num_send_frames(void) const { return _num_frames; }
};


libusb_zero_copy_impl::libusb_zero_copy_impl(usb_device_handle::sptr handle,
                                             unsigned int rx_endpoint,
                                             unsigned int tx_endpoint,
                                             size_t buff_size,
                                             size_t block_size)
 : _rx_ctx(NULL), _tx_ctx(NULL), _rx_dev_handle(NULL), _tx_dev_handle(NULL),
   _recv_buff_size(block_size), _send_buff_size(block_size),
   _num_frames(buff_size / block_size)
{
    libusb::init(&_rx_ctx, libusb_debug_level);
    libusb::init(&_tx_ctx, libusb_debug_level);

    UHD_ASSERT_THROW((_rx_ctx != NULL) && (_tx_ctx != NULL));

    _rx_dev_handle = libusb::open_device(_rx_ctx, handle);
    _tx_dev_handle = libusb::open_device(_tx_ctx, handle);

    libusb::open_interface(_rx_dev_handle, 2);
    libusb::open_interface(_tx_dev_handle, 1);

    _rx_ep = new usb_endpoint(_rx_dev_handle,
                              _rx_ctx,
                              rx_endpoint,
                              true,
                              _recv_buff_size,
                              _num_frames); 

    _tx_ep = new usb_endpoint(_tx_dev_handle,
                              _tx_ctx,
                              tx_endpoint,
                              false,
                              _send_buff_size,
                              _num_frames);
}


libusb_zero_copy_impl::~libusb_zero_copy_impl()
{
    delete _rx_ep;
    delete _tx_ep; 

    libusb_close(_rx_dev_handle);
    libusb_close(_tx_dev_handle);

    libusb_exit(_rx_ctx);
    libusb_exit(_tx_ctx);
}


managed_recv_buffer::sptr libusb_zero_copy_impl::get_recv_buff()
{
    libusb_transfer *lut = _rx_ep->get_completed_transfer();
    if (lut == NULL) {
        return managed_recv_buffer::sptr();
    }
    else {
        return managed_recv_buffer::sptr(
            new libusb_managed_recv_buffer_impl(lut,
                                                _rx_ep));
    }
}


managed_send_buffer::sptr libusb_zero_copy_impl::get_send_buff()
{
    libusb_transfer *lut = _tx_ep->get_free_transfer();
    if (lut == NULL) {
        return managed_send_buffer::sptr();
    }
    else {
        return managed_send_buffer::sptr(
            new libusb_managed_send_buffer_impl(lut,
                                                _tx_ep,
                                                _send_buff_size));
    }
}


/***********************************************************************
 * USB zero_copy make functions
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make(usb_device_handle::sptr handle,
                                        unsigned int rx_endpoint,
                                        unsigned int tx_endpoint,
                                        size_t buff_size,
                                        size_t block_size)

{
    return sptr(new libusb_zero_copy_impl(handle,
                                          rx_endpoint,
                                          tx_endpoint,
                                          buff_size, 
                                          block_size));
}



