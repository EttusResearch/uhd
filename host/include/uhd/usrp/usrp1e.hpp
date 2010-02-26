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

#ifndef INCLUDED_UHD_USRP_USRP1E_HPP
#define INCLUDED_UHD_USRP_USRP1E_HPP

#include <uhd/device.hpp>

namespace uhd{ namespace usrp{

/*!
 * The usrp1e device class.
 */
class usrp1e : public device{
public:
    /*!
     * Discover usrp1e devices on the system via the device node.
     * This static method will be called by the device::discover.
     * \param hint a device addr with the usrp1e address filled in
     * \return a vector of device addresses for all usrp1es found
     */
    static device_addrs_t discover(const device_addr_t &hint);

    /*!
     * Make a usrp1e from a device address.
     * \param addr the device address
     * \return a device sptr to a new usrp1e
     */
    static device::sptr make(const device_addr_t &addr);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_USRP1E_HPP */
