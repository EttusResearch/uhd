/* USRP E310 Firmware Texas Instruments BQ2419x driver
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
 * \file bq2419x.h
 * \brief Texas Instruments BQ2419x driver
 */

#ifndef BQ2419X_H
#define BQ2419X_H

#include <stdbool.h>
#include <stdint.h>

#include "pmu.h"
#include "interrupt.h"

typedef enum {
	BQ2419X_MODEL_24192 = 0x0,
	BQ2419X_MODEL_24191 = 0x1,
} bq2491x_model_t;

/**
 * \brief Initializes the BQ2419X chip
 */
int8_t bq2419x_init(void);

/**
 * \brief The IRQ handler for the CHG_IRQ external pin change interrupt
 * \return IRQ_HANDLED in case IRQ was handled, IRQ_NONE in case shared interrupt is not for us
 */
extern irqreturn_t bq2419x_irq_handler(void);

#endif /* BQ2419X_H */
