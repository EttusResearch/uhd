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

#ifndef INCLUDED_UHD_TYPES_BYTE_VECTOR_HPP
#define INCLUDED_UHD_TYPES_BYTE_VECTOR_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <boost/assign.hpp>
#include <boost/cstdint.hpp>

#include <uhd/config.hpp>

namespace uhd{

    //! Byte vector used for I2C data passing and EEPROM parsing.
    typedef std::vector<boost::uint8_t> byte_vector_t;

    template<typename RangeSrc, typename RangeDst> UHD_INLINE
    void byte_copy(const RangeSrc &src, RangeDst &dst){
        std::copy(boost::begin(src), boost::end(src), boost::begin(dst));
    }

    //! Create a string from a byte vector, terminate when invalid ASCII encountered
    UHD_API std::string bytes_to_string(const byte_vector_t &bytes);

    //! Create a byte vector from a string, end at null terminator or max length
    UHD_API byte_vector_t string_to_bytes(const std::string &str, size_t max_length);

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_BYTE_VECTOR_HPP */
