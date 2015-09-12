/* USRP E310 IO helpers
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
* \file io.h
* \brief IO helper functions to manipulate the MCUs pins
*/

#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>

#define IO_PX(port, pin) ((uint8_t)(((port - 'A') << 4) + pin))
#define IO_PA(pin) IO_PX('A', pin)
#define IO_PB(pin) IO_PX('B', pin)
#define IO_PC(pin) IO_PX('C', pin)
#define IO_PD(pin) IO_PX('D', pin)

typedef const uint8_t io_pin_t;

/**
 * \brief Make pin an output pin
 *
 * \param pin The pin to modify 
 */
void io_output_pin(io_pin_t pin);

/**
 * \brief Make pin an input pin
 *
 * \param pin The pin to modify
 */
void io_input_pin(io_pin_t pin);

/**
 * \brief Check if pin is an output
 *
 * \param pin The pin to query
 * \return Returns true if the pin is configured as output
 */
bool io_is_output(io_pin_t pin);

/**
 * \brief Check if pin is an input
 *
 * \param pin The pin to query
 * \return Returns true if the pin is configured as input
 */
bool io_is_input(io_pin_t pin);

/**
 * \brief If the pin is in input mode, this will enable the pull-up,
 *        if the pin is in output mode, this will set a logic high level 
 *
 * \param[in] pin The pin to modify
 */
void io_set_pin(io_pin_t pin);

/**
 * \brief If the pin is in input mode this will disable the pull-up
 *        if the pin is in output mode, this will set a logic low level
 *
 * \param[in] pin The pin to modify
 */
void io_clear_pin(io_pin_t pin);

/**
 * \brief If the pin is in input mode, this returns true if the pull-up is active,
 *        if the pin is in output mode, this returns true if a logic high is set
 * \param[in] pin The pin to query
 * \return True if pin is set, False otherwise
 */
bool io_is_pin_set(io_pin_t pin);


/**
 * \brief If the pin is in input mode, this returns the logic input value
 * \param[in] pin The pin to query
 * \return Returns true if a logic high is observed on the input pin
 */
bool io_test_pin(io_pin_t pin);

#endif /* IO_H */
