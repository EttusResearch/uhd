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

#include <uhd/usrp/dboard/manager.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "fw_common.h"

#ifndef INCLUDED_DBOARD_IMPL_HPP
#define INCLUDED_DBOARD_IMPL_HPP

/*!
 * The usrp2 dboard implementation:
 * Provide the properties access for a dboard.
 * Internally, hold a dboard manager and the direction.
 * The usrp2 mboard base implementation will create
 * two of these classes (one for rx and one for tx).
 */
class dboard_impl : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<dboard_impl> sptr;
    enum type_t {TYPE_RX, TYPE_TX};

    dboard_impl(uhd::usrp::dboard::manager::sptr manager, type_t type);
    
    ~dboard_impl(void);

    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

private:
    uhd::usrp::dboard::manager::sptr   _mgr;
    type_t                             _type;
};

#endif /* INCLUDED_DBOARD_IMPL_HPP */
