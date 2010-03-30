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

#ifndef INCLUDED_UHD_USRP_USRP_E_HPP
#define INCLUDED_UHD_USRP_USRP_E_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>

namespace uhd{ namespace usrp{

/*!
 * The USRP-Embedded device class.
 */
class UHD_API usrp_e : public device{
public:
    /*!
     * Find usrp_e devices on the system via the device node.
     * \param hint a device addr with the usrp_e address filled in
     * \return a vector of device addresses for all usrp-e's found
     */
    static device_addrs_t find(const device_addr_t &hint);

    /*!
     * Make a usrp_e from a device address.
     * \param addr the device address
     * \return a device sptr to a new usrp_e
     */
    static device::sptr make(const device_addr_t &addr);

    /*!
     * Load the FPGA with an image file.
     * \param bin_file the name of the fpga image file
     */
    static void load_fpga(const std::string &bin_file);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_USRP_E_HPP */
