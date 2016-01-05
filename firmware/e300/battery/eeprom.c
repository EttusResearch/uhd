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

#include "eeprom.h"
#include <avr/eeprom.h>

/* default values for eeprom */
char eeprom[] EEMEM = {0x00, 0x00, 0x00};

/* the avr libc wants it this way ... */
static uint8_t* EEPROM_AUTOBOOT_OFFSET		= (uint8_t *) 0x00;
static uint16_t *EEPROM_LAST_FULL_OFFSET	= (uint16_t *) 0x04;

static const uint8_t EEPROM_AUTOBOOT_MAGIC	= 0xa5;

bool eeprom_get_autoboot(void)
{
	return EEPROM_AUTOBOOT_MAGIC == eeprom_read_byte(EEPROM_AUTOBOOT_OFFSET);
}

void eeprom_set_autoboot(bool on)
{
	eeprom_update_byte(EEPROM_AUTOBOOT_OFFSET, on ? EEPROM_AUTOBOOT_MAGIC : 0x00);
}

uint16_t eeprom_get_last_full(void)
{
	return eeprom_read_word(EEPROM_LAST_FULL_OFFSET);
}

void eeprom_set_last_full_charge(uint16_t charge)
{
	eeprom_update_word(EEPROM_LAST_FULL_OFFSET, charge);
}
