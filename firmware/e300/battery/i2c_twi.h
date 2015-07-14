/* USRP E310 Firmware Atmel AVR TWI driver
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
 * \file i2c_twi.h
 * \brief Atmel AVR TWI driver
 */

#ifndef I2C_TWI_H
#define I2C_TWI_H

#include <stdlib.h>
#include <stdio.h>

/**
 * \brief Used for initializing the TWI/I2C module
 */
typedef enum i2c_speed_t {
	I2C_SPEED_100K,
	I2C_SPEED_400K,
} i2c_speed_t;

/**
 * \brief Initialize and calculate the TWBR value based on F_CPU and given rate
 *
 * \param[in] rate Target rate
 */
void i2c_twi_init_calc(uint32_t rate);

/**
 * \brief Initializes the AVR TWI/I2C module
 *
 * \param[in] speed Can be either 100KHz or 400Khz
 */
void i2c_twi_init(i2c_speed_t speed);

/**
 * \brief Read I2C register from I2C slave
 * \param[in] addr I2C slave address
 * \param[in] reg  Register address in slave register map
 * \param[out] value Output value
 * \return 0 on success, negative error code otherwise
 */
int8_t i2c_twi_read(uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * \brief Read 2 byte I2C register from I2C slave
 *
 * This is behaving a bit funny but is required for getting
 * the 2 byte values from the LTC294x chip.
 *
 * \param[in] addr I2C slave address
 * \param[in] reg  Register address in slave register map
 * \param[out] value Output value
 * \return 0 on success, negative error code otherwise
 */
int8_t i2c_twi_read16(uint8_t addr, uint8_t reg, uint16_t *value);

/**
 * \brief Write I2C register in I2C slave
 * \param[in] addr I2C slave address
 * \param[in] reg  Register address in slave register map
 * \param[in] value Value to be written
 * \return 0 on success, negative error code otherwise
 */
int8_t i2c_twi_write(uint8_t addr, uint8_t reg, uint8_t value);

/**
 * \brief Write 2 byte I2C register in I2C slave
 * \param[in] addr I2C slave address
 * \param[in] reg  Register address in slave register map
 * \param[in] value Value to be written
 * \return 0 on success, negative error code otherwise
 */
int8_t i2c_twi_write16(uint8_t addr, uint8_t reg, uint16_t value);

/**
 * \brief Handle SMBus alert response
 * \return 0 on success, negative error code otherwise
 */
uint8_t i2c_twi_smbus_ara(void);

#endif /* I2C_TWI_H */
