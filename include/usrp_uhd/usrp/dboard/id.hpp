//
// Copyright 2010 Ettus Research LLC
//

#include <iostream>

#ifndef INCLUDED_USRP_UHD_USRP_DBOARD_ID_HPP
#define INCLUDED_USRP_UHD_USRP_DBOARD_ID_HPP

namespace usrp_uhd{ namespace usrp{ namespace dboard{

enum dboard_id_t{
    ID_BASIC_TX = 0x0000,
    ID_BASIC_RX = 0x0001
};

}}} //namespace

std::ostream& operator<<(std::ostream &, const usrp_uhd::usrp::dboard::dboard_id_t &);

#endif /* INCLUDED_USRP_UHD_USRP_DBOARD_ID_HPP */
