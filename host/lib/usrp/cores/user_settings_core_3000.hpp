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

#ifndef INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class user_settings_core_3000 : public uhd::wb_iface {
public:
    virtual ~user_settings_core_3000() {}

    static sptr make(
        wb_iface::sptr iface,
        const wb_addr_type sr_base_addr, const wb_addr_type rb_reg_addr);
};

#endif /* INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP */
