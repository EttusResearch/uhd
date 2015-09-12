/* USRP E310 TP54478 driver
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

#include "mcu_settings.h"
#include "io.h"
#include "tps54478.h"

#include <stdlib.h>

#include <util/delay.h>
#include <util/atomic.h>

static io_pin_t CORE_PWR_EN = IO_PA(3);
static io_pin_t CORE_PGOOD = IO_PB(0);

/* per spec we should wait 3 ms here,
 * but 10 is better to give external PSU some time
 * to settle down*/
static const uint8_t TPS54478_START_DELAY = 10;

/* we'll use this to check for events in the event handler */
static volatile bool tps54478_event = false;

bool tps54478_get_power_good(void)
{
	return io_test_pin(CORE_PGOOD);
}

static int8_t tps54478_set_regulator(pmu_regulator_t *pmu_reg, bool on)
{
	(void) pmu_reg;

	if (on) {
		io_input_pin(CORE_PWR_EN);
		_delay_ms(TPS54478_START_DELAY);
	} else {
		io_output_pin(CORE_PWR_EN);
		/* no delay here as we can't detect this state anyway */
		return 0;
	}
	/* return zero on success ... */
	return !(on == tps54478_get_power_good());
}

void tps54478_init(bool enable)
{
	/* enable pull-up for open drain */
	io_input_pin(CORE_PGOOD);
	io_set_pin(CORE_PGOOD);

	tps54478_set_regulator(NULL, enable);

	io_clear_pin(CORE_PWR_EN);
}

int8_t tps54478_check_events(pmu_regulator_t *reg)
{
	bool power_good;
	bool event;

	event = false;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (tps54478_event) {
			tps54478_event = false;
			event = true;
		}
	}

	if (event) {
		power_good = tps54478_get_power_good();
		if (!power_good)
			return -1;
	}
	return 0;
}

const pmu_regulator_ops_t tps54478_ops = {
	.set_voltage = NULL,
	.set_regulator = tps54478_set_regulator,
	.check_events	= tps54478_check_events,
};

irqreturn_t tps54478_irq_handler(void)
{
	bool power_good;
	power_good = tps54478_get_power_good();

	/* check if the device indicates power is good,
	 * if it is probably we're not the source of the IRQ,
	 * if it is *not* set the event flag to deal with it later */
	if (power_good) {
		return IRQ_NONE;
	} else {
		tps54478_event = true;
		return IRQ_HANDLED;
	}
	return IRQ_HANDLED;
}
