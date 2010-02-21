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

#include <string>

#ifndef INCLUDED_UHD_USRP_DBOARD_ID_HPP
#define INCLUDED_UHD_USRP_DBOARD_ID_HPP

namespace uhd{ namespace usrp{

enum dboard_id_t{
    ID_NONE     = 0xffff,
    ID_BASIC_TX = 0x0000,
    ID_BASIC_RX = 0x0001
};

struct dboard_id{
    static std::string to_string(const dboard_id_t &id);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_ID_HPP */
