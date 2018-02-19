//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/types/serial.hpp>
#include <string>

namespace uhd{ namespace usrp{

struct UHD_API dboard_eeprom_t{

    //! The ID for the daughterboard type
    dboard_id_t id;

    //! The unique serial number
    std::string serial;

    //! A hardware revision number
    std::string revision;

    /*!
     * Create an empty dboard eeprom struct.
     */
    dboard_eeprom_t(void);

    /*!
     * Load the object with bytes from the eeprom.
     * \param iface the serial interface with i2c
     * \param addr the i2c address for the eeprom
     */
    void load(i2c_iface &iface, uint8_t addr);

    /*!
     * Store the object to bytes in the eeprom.
     * \param iface the serial interface with i2c
     * \param addr the i2c address for the eeprom
     */
    void store(i2c_iface &iface, uint8_t addr) const;

};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP */
