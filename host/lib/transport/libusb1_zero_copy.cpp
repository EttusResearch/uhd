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
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/shared_array.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace uhd::transport;

const int libusb_timeout = 0;

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
     * \param timeout_ms the timeout to wait for a lut
     * \return the transfer pointer or NULL if timeout
     */
    libusb_transfer *get_lut_with_wait(size_t timeout_ms = 100);

    //Callback use only
    void callback_handle_transfer(libusb_transfer *lut);

private:
    libusb::device_handle::sptr _handle;
    int  _endpoint;
    bool _input;

    size_t _transfer_size;
    size_t _num_transfers;

    //! hold a bounded buffer of completed transfers
    typedef bounded_buffer<libusb_transfer *> lut_buff_type;
    lut_buff_type::sptr _completed_list;

    //! a list of all transfer structs we allocated
    std::vector<libusb_transfer *>  _all_luts;

    //! a list of shared arrays for the transfer buffers
    std::vector<boost::shared_array<boost::uint8_t> > _buffers;

    // Calls for processing asynchronous I/O 
    libusb_transfer *allocate_transfer(int buff_len);
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
    _completed_list->push_with_wait(lut);
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
    _transfer_size(transfer_size),
    _num_transfers(num_transfers)
{
    _completed_list = lut_buff_type::make(num_transfers);

    for (size_t i = 0; i < _num_transfers; i++){
        _all_luts.push_back(allocate_transfer(_transfer_size));

        //input luts are immediately submitted to be filled
        //output luts go into the completed list as free buffers
        if (_input) this->submit(_all_luts.back());
        else _completed_list->push_with_wait(_all_luts.back());
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
    libusb_transfer *lut;
    while(_completed_list->pop_with_timed_wait(
        lut, boost::posix_time::milliseconds(100)
    ));

    //free all transfers
    BOOST_FOREACH(libusb_transfer *lut, _all_luts){
        libusb_free_transfer(lut);
    }
}


/*
 * Allocate a libusb transfer 
 * The allocated transfer - and buffer it contains - is repeatedly
 * submitted, reaped, and reused and should not be freed until shutdown.
 * \param buff_len size of the individual buffer held by each transfer
 * \return pointer to an allocated libusb_transfer
 */
libusb_transfer *usb_endpoint::allocate_transfer(int buff_len){
    libusb_transfer *lut = libusb_alloc_transfer(0);

    boost::shared_array<boost::uint8_t> buff(new boost::uint8_t[buff_len]);
    _buffers.push_back(buff); //store a reference to this shared array

    unsigned int endpoint = ((_endpoint & 0x7f) | (_input ? 0x80 : 0));
    libusb_transfer_cb_fn lut_callback = libusb_transfer_cb_fn(&callback);

    libusb_fill_bulk_transfer(lut,                // transfer
                              _handle->get(),     // dev_handle
                              endpoint,           // endpoint
                              buff.get(),         // buffer
                              buff_len,           // length
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

libusb_transfer *usb_endpoint::get_lut_with_wait(size_t timeout_ms){
    libusb_transfer *lut;
    if (_completed_list->pop_with_timed_wait(
        lut, boost::posix_time::milliseconds(timeout_ms)
    )) return lut;
    return NULL;
}

/***********************************************************************
 * Managed buffers 
 **********************************************************************/
/*
 * Libusb managed receive buffer
 * Construct a recv buffer from a libusb transfer. The memory held by
 * the libusb transfer is exposed through the managed buffer interface.
 * Upon destruction, the transfer and buffer are resubmitted to the
 * endpoint for further use. 
 */
class libusb_managed_recv_buffer_impl : public managed_recv_buffer {
public:
    libusb_managed_recv_buffer_impl(libusb_transfer *lut,
                                    usb_endpoint::sptr endpoint)
        : _buff(lut->buffer, lut->length)
    {
        _lut = lut;
        _endpoint = endpoint;
    }

    ~libusb_managed_recv_buffer_impl(void){
        _endpoint->submit(_lut);
    }

private:
    const boost::asio::const_buffer &get() const
    {
        return _buff; 
    }

    libusb_transfer *_lut;
    usb_endpoint::sptr _endpoint;
    const boost::asio::const_buffer _buff;
};

/*
 * Libusb managed send buffer
 * Construct a send buffer from a libusb transfer. The memory held by
 * the libusb transfer is exposed through the managed buffer interface.
 * Committing the buffer will set the data length and submit the buffer
 * to the endpoint. Submitting a buffer multiple times or destroying
 * the buffer before committing is an error. For the latter, the transfer
 * is returned to the endpoint with no data for reuse.
 */
class libusb_managed_send_buffer_impl : public managed_send_buffer {
public:
    libusb_managed_send_buffer_impl(libusb_transfer *lut,
                                    usb_endpoint::sptr endpoint,
                                    size_t buff_size)
        : _buff(lut->buffer, buff_size), _committed(false)
    {
        _lut = lut;
        _endpoint = endpoint;
    }

    ~libusb_managed_send_buffer_impl(void){
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

        try{
            _endpoint->submit(_lut);
            _committed = true;
            return num_bytes;
        }
        catch(const std::exception &e){
            std::cerr << "Error in commit: " << e.what() << std::endl;
            return -1;
        }
    }

private:
    const boost::asio::mutable_buffer &get() const
    {
        return _buff; 
    }

    libusb_transfer *_lut;
    usb_endpoint::sptr _endpoint;
    const boost::asio::mutable_buffer _buff;
    bool _committed;
};


/***********************************************************************
 * USB zero_copy device class
 **********************************************************************/
class libusb_zero_copy_impl : public usb_zero_copy
{
private:
    usb_endpoint::sptr _rx_ep, _tx_ep;

    libusb::device_handle::sptr _handle;

    size_t _recv_buff_size;
    size_t _send_buff_size;
    size_t _num_frames;

public:
    typedef boost::shared_ptr<libusb_zero_copy_impl> sptr;

    libusb_zero_copy_impl(libusb::device_handle::sptr handle,
                          unsigned int rx_endpoint,
                          unsigned int tx_endpoint,
                          size_t recv_buff_size,
                          size_t send_buff_size);

    managed_recv_buffer::sptr get_recv_buff(void);
    managed_send_buffer::sptr get_send_buff(void);

    size_t get_num_recv_frames(void) const { return _num_frames; }
    size_t get_num_send_frames(void) const { return _num_frames; }
};

/*
 * Constructor
 * Initializes libusb, opens devices, and sets up interfaces for I/O.
 * Finally, creates endpoints for asynchronous I/O. 
 */
libusb_zero_copy_impl::libusb_zero_copy_impl(libusb::device_handle::sptr handle,
                                             unsigned int rx_endpoint,
                                             unsigned int tx_endpoint,
                                             size_t buff_size,
                                             size_t block_size)
 : _handle(handle),
   _recv_buff_size(block_size), _send_buff_size(block_size),
   _num_frames(buff_size / block_size)
{
    _handle->claim_interface(2 /*in interface*/);
    _handle->claim_interface(1 /*out interface*/);

    _rx_ep = usb_endpoint::sptr(new usb_endpoint(
                              _handle,         // libusb device_handle
                              rx_endpoint,     // USB endpoint number
                              true,            // IN endpoint
                              _recv_buff_size, // buffer size per transfer 
                              _num_frames      // number of libusb transfers
    ));

    _tx_ep = usb_endpoint::sptr(new usb_endpoint(
                              _handle,         // libusb device_handle
                              tx_endpoint,     // USB endpoint number
                              false,           // OUT endpoint
                              _send_buff_size, // buffer size per transfer
                              _num_frames      // number of libusb transfers
    ));
}

/*
 * Construct a managed receive buffer from a completed libusb transfer
 * (happy with buffer full of data) obtained from the receive endpoint.
 * Return empty pointer if no transfer is available (timeout or error).
 * \return pointer to a managed receive buffer 
 */
managed_recv_buffer::sptr libusb_zero_copy_impl::get_recv_buff(void){
    libusb_transfer *lut = _rx_ep->get_lut_with_wait(/* TODO timeout API */);
    if (lut == NULL) {
        return managed_recv_buffer::sptr();
    }
    else {
        return managed_recv_buffer::sptr(
            new libusb_managed_recv_buffer_impl(lut,
                                                _rx_ep));
    }
}


/*
 * Construct a managed send buffer from a free libusb transfer (with
 * empty buffer). Return empty pointer of no transfer is available
 * (timeout or error).
 * \return pointer to a managed send buffer 
 */
managed_send_buffer::sptr libusb_zero_copy_impl::get_send_buff(void){
    libusb_transfer *lut = _tx_ep->get_lut_with_wait(/* TODO timeout API */);
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
    libusb::device_handle::sptr dev_handle(libusb::device_handle::get_cached_handle(
        boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
    ));
    return sptr(new libusb_zero_copy_impl(dev_handle,
                                          rx_endpoint,
                                          tx_endpoint,
                                          buff_size, 
                                          block_size));
}
