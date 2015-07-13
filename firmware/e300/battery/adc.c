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

#include <avr/io.h>

#include "adc.h"
#include "utils.h"

void adc_init(void)
{
	/* disable digital input on PC0 (ADC0) */
	DIDR0 |= 0x1;

	/* set to AVcc reference, left aligned and ADC0 */
	ADMUX = (1 << REFS0)
		| (0 << ADLAR)
		| (0 << MUX0);

	/* prescale clock by 128 */
	ADCSRA = BIT(ADPS2) | BIT(ADPS1) | BIT(ADPS0);
}

uint16_t adc_single_shot(void)
{
	uint16_t value;

	/* turn on ADC */
	ADCSRA |= (1 << ADEN);

	/* start conversion */
	ADCSRA |= (1 << ADSC);

	/* busy wait for conversion */
	while (ADCSRA & (1 << ADSC)) {
	};

	/* we need to first read the lower bits,
	 * which will lock the value until higher bits are read */
	value = (ADCL << 0);
	value |= (ADCH << 8);

	/* turn adc of again */
	ADCSRA &= ~(1 << ADEN);

	return value;
}
