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

#ifndef INCLUDED_LIBUHD_TRANSPORT_STDINT_H
#define INCLUDED_LIBUHD_TRANSPORT_STDINT_H

#include <boost/cstdint.hpp>

//provide a stdint implementation for libusb

typedef boost::uint64_t uint64_t;
typedef boost::uint32_t uint32_t;
typedef boost::uint16_t uint16_t;
typedef boost::uint8_t uint8_t;

typedef boost::int64_t int64_t;
typedef boost::int32_t int32_t;
typedef boost::int16_t int16_t;
typedef boost::int8_t int8_t;

#endif /* INCLUDED_LIBUHD_TRANSPORT_STDINT_H */
