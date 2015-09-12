/*
 * Copyright 2014-2015 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OCTOCLOCK_H_
#define _OCTOCLOCK_H_

#include "octoclock/common.h"

#include <stdbool.h>
#include <stdint.h>

// Define frequency
#define F_CPU 7372800UL

/*
 * Timer 1 (16-bit)
 *  * Set prescaler to 1024
 *  * Enable overflow interrupt
 *  * Set timer to 0
 */
#define TIMER1_INIT() TCCR1B = (1 << CS12) | (1 << CS10); \
                      TIMSK |= (1<<TOIE1); \
                      TCNT1 = 0;

#define TIMER1_DISABLE() TCCR1B = 0; \
                         TIMSK = 0; \
                         TCNT1 = 0;

#define TIMER1_ONE_SECOND ((uint32_t)(12207))

// Locations of OctoClock information in EEPROM
#define OCTOCLOCK_EEPROM_MAC_ADDR   0
#define OCTOCLOCK_EEPROM_IP_ADDR    6
#define OCTOCLOCK_EEPROM_DR_ADDR   10
#define OCTOCLOCK_EEPROM_NETMASK   14
#define OCTOCLOCK_EEPROM_SERIAL    18
#define OCTOCLOCK_EEPROM_NAME      28
#define OCTOCLOCK_EEPROM_REVISION  38

#define OCTOCLOCK_EEPROM_APP_LEN   100
#define OCTOCLOCK_EEPROM_APP_CRC   102

/* turn a numeric literal into a hex constant
 * (avoids problems with leading zeros)
 * 8-bit constants max value 0x11111111, always fits in unsigned long
 */
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
    +((x&0x000000F0LU)?2:0) \
    +((x&0x00000F00LU)?4:0) \
    +((x&0x0000F000LU)?8:0) \
    +((x&0x000F0000LU)?16:0) \
    +((x&0x00F00000LU)?32:0) \
    +((x&0x0F000000LU)?64:0) \
    +((x&0xF0000000LU)?128:0)

/* for up to 8-bit binary constants */
#define Bits_8(d) ((unsigned char)B8__(HEX__(d)))

/* for up to 16-bit binary constants, MSB first */
#define Bits_16(dmsb,dlsb) (((unsigned short)Bits_8(dmsb)<<8) \
    + Bits_8(dlsb))

/* for up to 32-bit binary constants, MSB first */
#define Bits_32(dmsb,db2,db3,dlsb) (((unsigned long)Bits_8(dmsb)<<24) \
    + ((unsigned long)Bits_8(db2)<<16) \
    + ((unsigned long)Bits_8(db3)<<8) \
    + Bits_8(dlsb))

/* Sample usage:
 * Bits_8(01010101) = 85
 * Bits_16(10101010,01010101) = 43605
 * Bits_32(10000000,11111111,10101010,01010101) = 2164238933
 */

void setup_atmel_io_ports(void);

#endif /* _OCTOCLOCK_H_ */
