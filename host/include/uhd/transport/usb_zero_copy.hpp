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

#include <uhd/config.hpp>
#include <uhd/types/usb_descriptor.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>

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
     * \param descriptor a USB descriptor identifying the device
     * \param rx_endpoint an integer specifiying an IN endpoint number 
     * \param tx_endpoint an integer specifiying an OUT endpoint number
     * \param buff_size total number of bytes of buffer space to allocate 
     * \param block_size number of bytes allocated for each I/O transaction 
     */
    static sptr make(usb_descriptor_t descriptor,
                     unsigned int rx_endpoint,
                     unsigned int tx_endpoint,
		     size_t buff_size = 0, 
                     size_t block_size = 0);
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_ZERO_COPY_HPP */
