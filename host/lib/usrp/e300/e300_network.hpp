//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_E300_NETWORK_HPP
#define INCLUDED_E300_NETWORK_HPP

#include <string>
#include <boost/noncopyable.hpp>

#include <uhd/device.hpp>


static const std::string E310_FPGA_FILE_NAME = "usrp_e310_fpga.bit";
static const std::string E300_FPGA_FILE_NAME = "usrp_e300_fpga.bit";

namespace uhd { namespace usrp { namespace e300 {

class UHD_API network_server : boost::noncopyable
{
public:
    typedef boost::shared_ptr<network_server> sptr;
    virtual void run(void) = 0;

    static sptr make(const uhd::device_addr_t &device_addr);
};


}}}
#endif // INCLUDED_E300_NETWORK_HPP
