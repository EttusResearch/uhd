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

#ifndef INCLUDED_UHD_TRANSPORT_USB_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_USB_ZERO_COPY_HPP

#include "usb_device_handle.hpp"
#include <uhd/transport/zero_copy.hpp>

namespace uhd { namespace transport {

/*!
 * A zero copy usb transport provides an efficient way to handle data.
 * by avoiding the extra copy when recv() or send() is called on the handle.
 * Rather, the zero copy transport gives the caller memory references.
 * The caller informs the transport when it is finished with the reference.
 *
 * On linux systems, the zero copy transport can use a kernel packet ring.
 * If no platform specific solution is available, make returns a boost asio
 * implementation that wraps functionality around standard send/recv calls.
 */
class UHD_API usb_zero_copy : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<usb_zero_copy> sptr;

    /*!
     * Make a new zero copy usb transport:
     * This transport is for sending and receiving between the host
     * and a pair of USB bulk transfer endpoints.
     * The primary usage for this transport is data transactions.
     * The underlying implementation may be platform specific.
     *
     * \param handle a device handle that uniquely identifying the device
     * \param recv_endpoint an integer specifiying an IN endpoint number
     * \param send_endpoint an integer specifiying an OUT endpoint number
     * \param recv_xfer_size the number of bytes for each receive transfer
     * \param recv_num_xfers the number of simultaneous receive transfers
     * \param send_xfer_size the number of bytes for each send transfer
     * \param send_num_xfers the number of simultaneous send transfers
     */
    static sptr make(
        usb_device_handle::sptr handle,
        unsigned int recv_endpoint,
        unsigned int send_endpoint,
        size_t recv_xfer_size = 0,
        size_t recv_num_xfers = 0,
        size_t send_xfer_size = 0,
        size_t send_num_xfers = 0
    );
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_ZERO_COPY_HPP */
