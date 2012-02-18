//
// Copyright 2012 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "wb_iface.hpp"

class user_settings_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<user_settings_core_200> sptr;
    typedef std::pair<boost::uint8_t, boost::uint32_t> user_reg_t;

    static sptr make(wb_iface::sptr iface, const size_t base);

    virtual void set_reg(const user_reg_t &reg) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP */
