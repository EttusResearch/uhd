//
// Copyright 2010,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/dboard_id.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iostream>

using namespace uhd::usrp;

dboard_id_t::dboard_id_t(uint16_t id){
    _id = id;
}

dboard_id_t dboard_id_t::none(void){
    return dboard_id_t();
}

dboard_id_t dboard_id_t::from_uint16(uint16_t uint16){
    return dboard_id_t(uint16);
}

uint16_t dboard_id_t::to_uint16(void) const{
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
        to_hex<uint16_t> hh;
        interpreter >> hh;
        return dboard_id_t::from_uint16(hh);
    }
    return dboard_id_t::from_uint16(boost::lexical_cast<uint16_t>(string));
}

std::string dboard_id_t::to_string(void) const{
    return str(boost::format("0x%04x") % this->to_uint16());
}

//Note: to_pp_string is implemented in the dboard manager
//because it needs access to the dboard registration table

bool uhd::usrp::operator==(const dboard_id_t &lhs, const dboard_id_t &rhs){
    return lhs.to_uint16() == rhs.to_uint16();
}
