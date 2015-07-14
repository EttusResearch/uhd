/* USRP E310 Firmware Linear Technology LTC294X driver
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
 * \file ltc294x.h
 * \brief Linear Technology LTC294X driver
 */

#ifndef LTC294X_H
#define LTC294X_H

#include "pmu.h"

typedef enum {
	LTC294X_MODEL_2941 = 0x1,
	LTC294X_MODEL_2942 = 0x0
} ltc294x_model_t;

/**
 * \brief Initializes the LTC294X chip
 * \param model What model we're looking we should probe for
 * \return 0 on success, negative error code otherwise
 */
int8_t ltc294x_init(ltc294x_model_t model);

extern irqreturn_t ltc294x_irq_handler(void);

#endif /* LTC294X_H */
