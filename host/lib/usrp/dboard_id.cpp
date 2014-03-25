//
// Copyright 2010,2014 Ettus Research LLC
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

#include <uhd/usrp/dboard_id.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iostream>

using namespace uhd::usrp;

dboard_id_t::dboard_id_t(boost::uint16_t id){
    _id = id;
}

dboard_id_t dboard_id_t::none(void){
    return dboard_id_t();
}

dboard_id_t dboard_id_t::from_uint16(boost::uint16_t uint16){
    return dboard_id_t(uint16);
}

boost::uint16_t dboard_id_t::to_uint16(void) const{
    return _id;
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

dboard_id_t dboard_id_t::from_string(const std::string &string){
    if (string.substr(0, 2) == "0x"){
        std::stringstream interpreter(string);
        to_hex<boost::uint16_t> hh;
        interpreter >> hh;
        return dboard_id_t::from_uint16(hh);
    }
    return dboard_id_t::from_uint16(boost::lexical_cast<boost::uint16_t>(string));
}

std::string dboard_id_t::to_string(void) const{
    return str(boost::format("0x%04x") % this->to_uint16());
}

//Note: to_pp_string is implemented in the dboard manager
//because it needs access to the dboard registration table

bool uhd::usrp::operator==(const dboard_id_t &lhs, const dboard_id_t &rhs){
    return lhs.to_uint16() == rhs.to_uint16();
}
