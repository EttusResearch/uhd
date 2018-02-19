//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP

#include <uhd/types/dict.hpp>
#include <string>

namespace uhd{ namespace usrp{

    /*!  The motherboard EEPROM object.
     *
     * The specific implementation knows how to read and write the EEPROM for
     * various USRPs. By itself, this class is nothing but a thin wrapper
     * around a string -> string dictionary.
     *
     * Note that writing to an object of type mboard_eeprom_t does not actually
     * write to the EEPROM. Devices have their own APIs to read/write from the
     * EEPROM chips themselves. Most devices will write the EEPROM itself when
     * the according property is updated.
     */
    typedef uhd::dict<std::string, std::string> mboard_eeprom_t;

}} // namespace uhd::usrp

#endif /* INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP */
