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

#include <uhd/device.hpp>
#include <uhd/usrp/mboard/base.hpp>
#include <map>

#ifndef INCLUDED_UHD_USRP_USRP_HPP
#define INCLUDED_UHD_USRP_USRP_HPP

namespace uhd{ namespace usrp{

/*!
 * A usrp device provides a device-level interface to usrp mboards.
 * In most cases, a usrp device will have only one mboard.
 * In the usrp2 mimo case, this device will have two mboards,
 * where one talks through the other's control port.
 */
class usrp : public device{
public:
    usrp(const device_addr_t & device_addr);
    ~usrp(void);

    //the io interface
    void send_raw(const std::vector<boost::asio::const_buffer> &);
    const boost::asio::const_buffer recv_raw(void);

private:
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    std::map<std::string, mboard::base::sptr> _mboards;
    boost::function<void(const std::vector<boost::asio::const_buffer> &)> _send_raw_cb;
    boost::function<const boost::asio::const_buffer(void)> _recv_raw_cb;
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_USRP_HPP */
