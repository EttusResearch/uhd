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

#ifndef INCLUDED_UHD_USRP_MBOARD_USRP2_HPP
#define INCLUDED_UHD_USRP_MBOARD_USRP2_HPP

#include <uhd/usrp/mboard/base.hpp>
#include <uhd/device_addr.hpp>

namespace uhd{ namespace usrp{ namespace mboard{

/*!
 * The usrp2 mboard class.
 */
class usrp2 : public base{
public:
    /*!
     * Discover usrp2 devices over the ethernet.
     * This static method will be called by the device::discover.
     * \param hint a device addr with the usrp2 address filled in
     * \return a vector of device addresses for all usrp2s found
     */
    static device_addrs_t discover(const device_addr_t &hint);

    usrp2(const device_addr_t &);
    ~usrp2(void);

private:
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    wax::obj _impl;
};

}}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_USRP2_HPP */
