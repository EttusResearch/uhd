//
// Copyright 2011-2012 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "wb_iface.hpp"

class i2c_core_200 : boost::noncopyable, public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<i2c_core_200> sptr;

    //! makes a new i2c core from iface and slave base
    static sptr make(wb_iface::sptr iface, const size_t base, const size_t readback);
};

#endif /* INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP */
