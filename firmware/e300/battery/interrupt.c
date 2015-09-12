/* USRP E310 Firmware Interrupt Management
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

#include <stdint.h>
#include <avr/interrupt.h>

#include "utils.h"
#include "interrupt.h"

#include "bq2419x.h"
#include "tps54478.h"
#include "ltc3675.h"
#include "ltc294x.h"
#include "pmu.h"
#include "led.h"

static const irq_handler_t pcint0_irqs[] = {bq2419x_irq_handler};
static const irq_handler_t pcint1_irqs[] = {NULL};
static const irq_handler_t pcint2_irqs[] = {NULL};
static const irq_handler_t pcint3_irqs[] = {NULL};
static const irq_handler_t int0_handler = ltc3675_button_wakeup_irq_handler;
static const irq_handler_t int1_handler = ltc3675_button_change_irq_handler;
//static const irq_handler_t timer0_comp_a_handler = pmu_led_timer_comp_a_irq_handler;
//static const irq_handler_t timer0_comp_b_handler = pmu_led_timer_comp_b_irq_handler;
static const irq_handler_t timer1_handler = ltc3675_button_timer_irq_handler;
static const irq_handler_t wdt_handler = {led_wdt_handler};

void interrupt_init(void)
{
	/* rising edge for WAKEUP and any change for ONSWITCH_DB */
	EICRA = BIT(ISC01) | BIT(ISC00) | BIT(ISC10);

	/* enable interrupt for WAKE  */
	EIMSK = BIT(INT1) | BIT(INT0);

	/* enable interrupt for CORE_PGOOD and CHG_IRQ */
	PCMSK0 = /*BIT(PCINT0) | */ BIT(PCINT1);

	/* enable interrupts for PWR_IRQ and AVR_IRQ */
	PCMSK2 = BIT(PCINT16) | BIT(PCINT21);

	/* enable interrupts for FC_ALn_CC */
	//PCMSK3 = BIT(PCINT24);

	/* unmask IRQs for PC[23:16] and PC[7:0] */
	PCICR = BIT(PCIE3) | BIT(PCIE2) | BIT(PCIE0);
}

ISR(PCINT0_vect)
{
	uint8_t i;
	irqreturn_t ret;

	for (i = 0; i < ARRAY_SIZE(pcint0_irqs); i++) {
		irq_handler_t handler = pcint0_irqs[i];
		if (handler != NULL) {
			ret = handler();
			if (ret == IRQ_HANDLED)
				break;
		}
	}
}

ISR(PCINT1_vect)
{
	uint8_t i;
	irqreturn_t ret;

	for (i = 0; i < ARRAY_SIZE(pcint1_irqs); i++) {
		irq_handler_t handler = pcint1_irqs[i];
		if (handler != NULL) {
			ret = handler();
			if (ret == IRQ_HANDLED)
				break;
		}
	}
}

ISR(PCINT2_vect)
{
	uint8_t i;
	irqreturn_t ret;

	for (i = 0; i < ARRAY_SIZE(pcint2_irqs); i++) {
		irq_handler_t handler = pcint2_irqs[i];
		if (handler != NULL) {
			ret = handler();
			if (ret == IRQ_HANDLED)
				break;
		}
	}
}

ISR(PCINT3_vect)
{
	uint8_t i;
	irqreturn_t ret;

	for (i = 0; i < ARRAY_SIZE(pcint3_irqs); i++) {
		irq_handler_t handler = pcint3_irqs[i];
		if (handler != NULL) {
			ret = handler();
			if (ret == IRQ_HANDLED)
				break;
		}
	}
}

ISR(INT0_vect)
{
	if (int0_handler)
		int0_handler();
}

ISR(INT1_vect)
{
	if (int1_handler) {
		int1_handler();
	}
}

/*
ISR(TIMER0_COMPA_vect)
{
	timer0_comp_a_handler();
}

ISR(TIMER0_COMPB_vect)
{
	timer0_comp_b_handler();
}
*/

ISR(TIMER1_COMPA_vect)
{
	timer1_handler();
}

ISR(WDT_vect)
{
	wdt_handler();
}
