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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <uhd/types/byte_vector.hpp>

namespace uhd{

std::string bytes_to_string(const byte_vector_t &bytes){
    std::string out;
    BOOST_FOREACH(boost::uint8_t byte, bytes){
        if (byte < 32 or byte > 127) return out;
        out += byte;
    }
    return out;
}

std::string uint16_bytes_to_string(const byte_vector_t &bytes){
    const boost::uint16_t num = (boost::uint16_t(bytes.at(0)) << 0) | (boost::uint16_t(bytes.at(1)) << 8);
    return (num == 0 or num == 0xffff)? "" : boost::lexical_cast<std::string>(num);
}

byte_vector_t string_to_bytes(const std::string &str, size_t max_length){
    byte_vector_t bytes;
    for (size_t i = 0; i < std::min(str.size(), max_length); i++){
        bytes.push_back(str[i]);
    }
    if (bytes.size() < max_length - 1) bytes.push_back('\0');
    return bytes;
}

byte_vector_t string_to_uint16_bytes(const std::string &num_str){
    const boost::uint16_t num = boost::lexical_cast<boost::uint16_t>(num_str);
    const byte_vector_t lsb_msb = boost::assign::list_of
        (boost::uint8_t(num >> 0))(boost::uint8_t(num >> 8));
    return lsb_msb;
}

} /* namespace uhd */
