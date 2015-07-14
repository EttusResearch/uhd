/* USRP E310 Firmware EEPROM driver
 * Copyright (C) 2014 Ettus Research
 * This file is part of the USRP E310 Firmware
 * The USRP E310 Firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * The USRP E310 Firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with the USRP E310 Firmware. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file eeprom.h
 * \brief AVR EEPROM driver
 */

#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

/**
 * \brief Get the value for the autoboot flag set in the EEPROM
 *
 */
bool eeprom_get_autoboot(void);

/**
 * \brief Set the value for the autoboot flag set in the EEPROM
 *
 * \param[in] on value to write to EEPROM. Use 'true' to turn on autoboot.
 *
 */
void eeprom_set_autoboot(bool on);

/**
 * \brief Get last full charge from the EEPROM
 *
 */
uint16_t eeprom_get_last_full(void);

/**
 * \brief Set last full charge in the EEPROM
 *
 * \param[in] charge value to write to EEPROM.
 */
void eeprom_set_last_full_charge(uint16_t charge);

#endif /* EEPROM_H */
