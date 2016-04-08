//
// Copyright 2010-2013 Ettus Research LLC
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

#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>

namespace uhd { namespace transport {

/*!
 * A zero copy USB transport provides an efficient way to handle data.
 * by avoiding the extra copy when recv() or send() is called on the handle.
 * Rather, the zero copy transport gives the caller memory references.
 * The caller informs the transport when it is finished with the reference.
 *
 * On Linux systems, the zero copy transport can use a kernel packet ring.
 * If no platform specific solution is available, make returns a boost asio
 * implementation that wraps functionality around standard send/recv calls.
 */
class UHD_API usb_zero_copy : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<usb_zero_copy> sptr;

    /*!
     * Make a new zero copy USB transport:
     * This transport is for sending and receiving between the host
     * and a pair of USB bulk transfer endpoints.
     * The primary usage for this transport is data transactions.
     * The underlying implementation may be platform specific.
     *
     * \param handle a device handle that uniquely identifying the device
     * \param recv_interface an integer specifying an IN interface number
     * \param recv_endpoint an integer specifying an IN endpoint number
     * \param send_interface an integer specifying an OUT interface number
     * \param send_endpoint an integer specifying an OUT endpoint number
     * \param hints optional parameters to pass to the underlying transport
     * \return a new zero copy USB object
     */
    static sptr make(
        usb_device_handle::sptr handle,
        const size_t recv_interface,
        const size_t recv_endpoint,
        const size_t send_interface,
        const size_t send_endpoint,
        const device_addr_t &hints = device_addr_t()
    );
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_ZERO_COPY_HPP */
