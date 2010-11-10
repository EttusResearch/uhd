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

#include "mboard_rev.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iostream>

static const mboard_rev_t usrp2p_first_hw_rev = mboard_rev_t(0x0A00);

mboard_rev_t::mboard_rev_t(boost::uint16_t rev){
    _rev = rev;
}

mboard_rev_t mboard_rev_t::none(void){
    return mboard_rev_t();
}

mboard_rev_t mboard_rev_t::from_uint16(boost::uint16_t uint16){
    return mboard_rev_t(uint16);
}

boost::uint16_t mboard_rev_t::to_uint16(void) const{
    return _rev;
}

//used with lexical cast to parse a hex string
template <class T> struct to_hex{
    T value;
    operator T() const {return value;}
    friend std::istream& operator>>(std::istream& in, to_hex& out){
        in >> std::hex >> out.value;
        return in;
    }
};

mboard_rev_t mboard_rev_t::from_string(const std::string &string){
    if (string.substr(0, 2) == "0x"){
        return mboard_rev_t::from_uint16(boost::lexical_cast<to_hex<boost::uint16_t> >(string));
    }
    return mboard_rev_t::from_uint16(boost::lexical_cast<boost::uint16_t>(string));
}

std::string mboard_rev_t::to_string(void) const{
    return str(boost::format("0x%04x") % this->to_uint16());
}

std::string mboard_rev_t::to_pp_string(void) const{
    if(this->is_usrp2p()) {
        return str(boost::format("USRP2+, major rev %i, minor rev %i") % int(this->major()) % int(this->minor()));
    } else {
        return str(boost::format("USRP2, major rev %i, minor rev %i") % int(this->major()) % int(this->minor()));
    }
}

bool mboard_rev_t::is_usrp2p(void) const{
    return _rev >= usrp2p_first_hw_rev;
}

boost::uint8_t mboard_rev_t::major(void) const{
    return _rev >> 8;
}

boost::uint8_t mboard_rev_t::minor(void) const{
    return _rev & 0xff;
}

bool operator==(const mboard_rev_t &lhs, const mboard_rev_t &rhs){
    return lhs.to_uint16() == rhs.to_uint16();
}

bool operator<(const mboard_rev_t &lhs, const mboard_rev_t &rhs){
    return lhs.to_uint16() < rhs.to_uint16();
}
