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

#ifndef INCLUDED_UHD_USRP_MBOARD_BASE_HPP
#define INCLUDED_UHD_USRP_MBOARD_BASE_HPP

#include <uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace usrp{ namespace mboard{

/*!
 * A base class for usrp mboard objects.
 */
class base : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<base> sptr;
    base(void);
    ~base(void);

    //TODO other api calls

private:
    virtual void get(const wax::obj &, wax::obj &) = 0;
    virtual void set(const wax::obj &, const wax::obj &) = 0;
};

}}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_BASE_HPP */
