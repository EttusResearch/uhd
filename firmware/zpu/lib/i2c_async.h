//
// Copyright 2010 Ettus Research LLC
//
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#ifndef INCLUDED_I2C_ASYNC_H
#define INCLUDED_I2C_ASYNC_H

#include <stdbool.h>
#include "stdint.h"

typedef enum { I2C_STATE_IDLE, 
               I2C_STATE_CONTROL_BYTE_SENT, 
               I2C_STATE_DATA, 
               I2C_STATE_LAST_BYTE, 
               I2C_STATE_DATA_READY, 
               I2C_STATE_ERROR 
             } i2c_state_t;

typedef enum { I2C_DIR_WRITE=0, I2C_DIR_READ=1 } i2c_dir_t;

bool i2c_async_read(uint8_t addr, unsigned int len);
bool i2c_async_write(uint8_t addr, const uint8_t *buf, unsigned int len);
bool i2c_async_data_ready(void *);
//static void i2c_irq_handler(unsigned irq);
void i2c_register_callback(void (*callback)(void));
void i2c_register_handler(void);

// Write 24LC024 / 24LC025 EEPROM on motherboard or daughterboard.
// Which EEPROM is determined by i2c_addr.  See i2c_addr.h

bool eeprom_write_async (int i2c_addr, int eeprom_offset, const void *buf, int len, void (*callback)(void));
bool eeprom_read_async(int i2c_addr, int eeprom_offset, void *buf, int len, void (*callback)(void));

#endif /* INCLUDED_I2C_ASYNC_H */
