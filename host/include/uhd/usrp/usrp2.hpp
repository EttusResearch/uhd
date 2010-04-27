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

#ifndef INCLUDED_UHD_USRP_USRP2_HPP
#define INCLUDED_UHD_USRP_USRP2_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>

namespace uhd{ namespace usrp{

/*!
 * The usrp2 device class.
 */
class UHD_API usrp2 : public device{
public:
    /*!
     * Find usrp2 devices over the ethernet.
     *
     * Recommended key/value pairs for the device hint address:
     * hint["addr"] = address, where address is a resolvable address
     * or ip address, which may or may not be a broadcast address.
     *
     * Other optional device address keys:
     *   recv_buff_size: resizes the recv buffer on the data socket
     *   send_buff_size: resizes the send buffer on the data socket
     *
     * \param hint a device addr with the usrp2 address filled in
     * \return a vector of device addresses for all usrp2s found
     */
    static device_addrs_t find(const device_addr_t &hint);

    /*!
     * Make a usrp2 from a device address.
     *
     * Required key/value pairs for the device address:
     * hint["addr"] = address, where address is a resolvable address
     * or ip address, which must be the specific address of a usrp2.
     *
     * \param addr the device address
     * \return a device sptr to a new usrp2
     */
    static device::sptr make(const device_addr_t &addr);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_USRP2_HPP */
