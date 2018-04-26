//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_BYTE_VECTOR_HPP
#define INCLUDED_UHD_TYPES_BYTE_VECTOR_HPP

#include <uhd/config.hpp>
#include <boost/range.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace uhd{

    //! Byte vector used for I2C data passing and EEPROM parsing.
    typedef std::vector<uint8_t> byte_vector_t;

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
