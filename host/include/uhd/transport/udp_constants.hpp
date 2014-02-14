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

#ifndef INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP
#define INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP

// Constants related to UDP (over Ethernet)

static const size_t IP_PROTOCOL_MIN_MTU_SIZE        = 576;      //bytes
static const size_t IP_PROTOCOL_UDP_PLUS_IP_HEADER  = 28;      //bytes. Note that this is the minimum value!

#endif /* INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP */
