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

#ifndef INCLUDED_UHD_USRP_MBOARD_TEST_HPP
#define INCLUDED_UHD_USRP_MBOARD_TEST_HPP

#include <uhd/usrp/mboard/base.hpp>
#include <uhd/device_addr.hpp>
#include <uhd/usrp/dboard/manager.hpp>
#include <map>

namespace uhd{ namespace usrp{ namespace mboard{

/*!
 * A test usrp mboard object.
 * Exercises access routines for the test suite.
 */
class test : public base{
public:
    test(const device_addr_t &);
    ~test(void);

private:
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    std::map<std::string, dboard::manager::sptr> _dboard_managers;
};

}}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_TEST_HPP */
