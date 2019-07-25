//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_EEPROM_HPP
#define INCLUDED_X300_EEPROM_HPP

#include <uhd/types/serial.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>

namespace uhd { namespace usrp { namespace x300 {

//! Read out the on-board EEPROM, convert to dict, and return
uhd::usrp::mboard_eeprom_t get_mb_eeprom(uhd::i2c_iface::sptr i2c);

//! Write the contents of an EEPROM dict to the on-board EEPROM
void set_mb_eeprom(
    uhd::i2c_iface::sptr iface, const uhd::usrp::mboard_eeprom_t& mb_eeprom);

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_EEPROM_HPP */
