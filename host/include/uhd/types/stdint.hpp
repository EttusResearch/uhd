//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_STDINT_HPP
#define INCLUDED_UHD_TYPES_STDINT_HPP

#include <boost/cstdint.hpp>

using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;

using boost::int_least8_t;
using boost::uint_least8_t;
using boost::int_least16_t;
using boost::uint_least16_t;
using boost::int_least32_t;
using boost::uint_least32_t;
using boost::int_least64_t;
using boost::uint_least64_t;

using boost::int_fast8_t;
using boost::uint_fast8_t;
using boost::int_fast16_t;
using boost::uint_fast16_t;
using boost::int_fast32_t;
using boost::uint_fast32_t;
using boost::int_fast64_t;
using boost::uint_fast64_t;

using ::intptr_t;
using ::uintptr_t;

#endif /* INCLUDED_UHD_TYPES_STDINT_HPP */
