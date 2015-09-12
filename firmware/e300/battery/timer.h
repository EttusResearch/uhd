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

/**
 * \file timer.h
 * \brief Timer driver
 */
#ifndef TIMER_H
#define TIMER_H

/**
 * \brief Initialize Timer 0
 */
void timer0_init(void);

/**
 * \brief Start Timer 0
 */
void timer0_start(void);

/**
 * \brief Stop Timer 0
 */
void timer0_stop(void);

/**
 * \brief Initialize Timer 1
 */
void timer1_init(void);

/**
 * \brief Start Timer 1
 */
void timer1_start(void);

/**
 * \brief Stop Timer 1
 */
void timer1_stop(void);

#endif /* TIMER_H */
