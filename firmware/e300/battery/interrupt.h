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

#ifndef INTERRUPT_H
#define INTERRUPT_H

/**
 * \brief Shared IRQ handlers return either of these values
 *
 * A shared IRQ handler will either 'claim' the IRQ and return IRQ_HANDLED,
 * or indicate it is not sure whether it was the IRQ source and return IRQ_NONE.
 */
typedef enum {
	IRQ_NONE,
	IRQ_HANDLED
} irqreturn_t;

/**
 * \brief (Shared) IRQ handlers should use this type signature
 */
typedef irqreturn_t (*irq_handler_t)(void);

/**
 * \brief Initialize the IRQ subsystem 
 */
void interrupt_init(void);

#endif /* INTERRUPT_H */
