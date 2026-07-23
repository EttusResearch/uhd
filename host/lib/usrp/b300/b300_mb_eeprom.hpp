//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/serial.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>

namespace uhd { namespace usrp { namespace b300 {

//! Convert PID to product name.
std::string map_pid_to_product_name(const uint32_t pid);

//! Read out the on-board EEPROM, convert to dict, and return
uhd::usrp::mboard_eeprom_t get_mb_eeprom(uhd::i2c_iface::sptr i2c);

//! Write the contents of an EEPROM dict to the on-board EEPROM
void set_mb_eeprom(
    uhd::i2c_iface::sptr iface, const uhd::usrp::mboard_eeprom_t& mb_eeprom);

}}} // namespace uhd::usrp::b300
