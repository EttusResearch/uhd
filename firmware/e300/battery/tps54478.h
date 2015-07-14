/* USRP E310 Firmware Texas Instruments TPS54478 driver
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
 * \file tps54478.h
 * \brief Texas Instruments TPS54478 driver
 */

#ifndef TPS54478_H
#define TPS54478_H

#include <stdbool.h>
#include "pmu.h"
#include "interrupt.h"

/**
 * \brief Initializes the TPS54478 controlling the core power supply
 * \param[in] enabled Controls whether the core power gets turned on
 */
void tps54478_init(bool enabled);

/**
 * \brief The IRQ handler for the CHG_IRQ external pin change interrupt
 * \return IRQ_HANDLED in case IRQ was handled, IRQ_NONE in case shared interrupt is not for us
 */
extern irqreturn_t tps54478_irq_handler(void);

typedef struct tps54478_pmu_regulator {
	pmu_regulator_t pmu_reg;
} tps54478_pmu_regulator_t;

extern const pmu_regulator_ops_t tps54478_ops;

#endif /* TPS54478_H */
