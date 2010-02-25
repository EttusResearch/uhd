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

#include <uhd/usrp/usrp1e.hpp>

using namespace uhd;
using namespace uhd::usrp;

/*!
 * This file defines the usrp1e discover and make functions
 * when the required kernel module headers are not present.
 */

device_addrs_t usrp1e::discover(const device_addr_t &){
    return device_addrs_t(); //return empty list
}

device::sptr usrp1e::make(const device_addr_t &){
    throw std::runtime_error("this build has no usrp1e support");
}
