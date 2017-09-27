//
// Copyright 2016 Ettus Research LLC
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

#ifndef INCLUDED_X300_MB_EEPROM_HPP
#define INCLUDED_X300_MB_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class x300_mb_eeprom_iface : public uhd::i2c_iface
{
public:
    typedef boost::shared_ptr<x300_mb_eeprom_iface> sptr;

    virtual ~x300_mb_eeprom_iface(void) = 0;

    static sptr make(uhd::wb_iface::sptr wb, uhd::i2c_iface::sptr i2c);
};

#endif /* INCLUDED_X300_MB_EEPROM_HPP */
