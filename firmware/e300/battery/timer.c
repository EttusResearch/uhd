/* USRP E310 Firmware Timer driver
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

#include "timer.h"
#include "utils.h"


static const uint8_t TIMER0_OFF_MASK = 0xf8;
static const uint8_t TIMER1_OFF_MASK = 0xf8;

void timer0_init(void)
{
	/* ctc mode */
	TCCR0A = BIT(CTC0);

	/* 250 ms with 1024 prescale @ 1 MHz */
	OCR0A = 244;//244;

	/* ctc mode */
	TIMSK0 = BIT(OCIE0A);
}

void timer0_start(void)
{
	TCNT0 = 0x00;
	/* set prescaler to 1024 */
	TCCR0A |= BIT(CS02) | BIT(CS00);
}

void timer0_stop(void)
{
	/* mask out all the bits to stop */
	TCCR0A &= TIMER0_OFF_MASK;
}

void timer1_init(void)
{
	/* set counter register to 0 */
	TCNT1 = 0x0;

	/* ctc mode */
	TCCR1B = BIT(WGM12);

	/* hold button for roughly 2 seconds
	 * value is calculated as follows:
	 * val = 2 * f_clk / (prescaler * f_irq) - 1
	 */
	OCR1A = 7811 * 2;

	/* enable CTC on timer 1 */
	TIMSK1 = BIT(OCIE1A);
}

void timer1_start(void)
{
	/* reset counter register */
	TCNT1 = 0x0000;

	/* set prescaler to 1024 */
	TCCR1B |= BIT(CS12) | BIT(CS10);
}

void timer1_stop(void)
{
	/* mask out all the bits to stop */
	TCCR1B &= TIMER1_OFF_MASK;
}

