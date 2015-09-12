/*
 * Copyright 2014 Ettus Research LLC
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

#ifndef _CLKDIST_H_
#define _CLKDIST_H_

#include <stdbool.h>

#include <octoclock.h>

typedef enum {
    Reg0=0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7,
    Reg8_Status_Control,
    Read_Command=0xE,
    RAM_EEPROM_Unlock=0x1F,
    RAM_EEPROM_Lock=0x3f
} CDCE18005;

typedef enum {
    Primary_GPS,
    Secondary_Ext
} TI_Input_10_MHz;

typedef enum {
    Lo,
    Hi
} Levels;

void setup_TI_CDCE18005(TI_Input_10_MHz which_input);

void reset_TI_CDCE18005(void);

uint32_t get_TI_CDCE18005(CDCE18005 which_register);

void set_TI_CDCE18005(CDCE18005 which_register, uint32_t bits);

bool check_TI_CDCE18005(TI_Input_10_MHz which_input, CDCE18005 which_register);

#endif /* _CLKDIST_H_ */
