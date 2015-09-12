/* USRP E310 Firmware Linear Technology LTC3765 driver
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
 * \file ltc3675.h
 * \brief Linear Technology LTC3675 driver
 */

#ifndef LTC3675_H
#define LTC3675_H

#include "pmu.h"

typedef enum ltc3675_regulator {
	/** 1A Buck */
	LTC3675_REG_1,
	/** 1A Buck */
	LTC3675_REG_2,
	/** 500mA Buck */
	LTC3675_REG_3,
	/** 500mA Buck */
	LTC3675_REG_4,
	/** 1A Boost */
	LTC3675_REG_5,
	/** 1A Buck-Boost */
	LTC3675_REG_6,
} ltc3675_regulator_t;

typedef struct ltc3675_pmu_regulator {
	pmu_regulator_t pmu_reg;
	ltc3675_regulator_t ltc3675_reg;
} ltc3675_pmu_regulator_t;

extern const pmu_regulator_ops_t ltc3675_ops;

/**
 * \brief Initializes the LTC3675 chip
 *
 * This function will setup the internal pull-up resistors for the open drain
 * input pins, clear old interrupts, unmask all warnings and set the warning
 * level to 3.4V
 */
int8_t ltc3675_init(void);

/**
 * \brief Event handler that gets called periodically
 * \return returns 0 on success, negative error code in case of fault
 */
int8_t ltc3675_handle_events(void);

extern irqreturn_t ltc3675_button_wakeup_irq_handler(void);
extern irqreturn_t ltc3675_button_change_irq_handler(void);
extern irqreturn_t ltc3675_button_timer_irq_handler(void);

#endif /* LTC3675_H */
