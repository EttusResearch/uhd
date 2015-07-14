/* USRP E310 Firmware Atmel AVR ADC driver
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
 * \file adc.h
 * \brief Atmel AVR ADC driver
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/**
 * \brief Initialize the ADC for conversion on PC0 (ADC0)
 *        with a prescaler of 128, and AVcc reference.
 */
void adc_init(void);

/**
 * \brief Do a single shot conversion on PC0 (ADC0)
 * \return Value of ADC
 */
uint16_t adc_single_shot(void);

#endif /* ADC_H */
