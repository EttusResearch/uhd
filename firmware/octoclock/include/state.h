/*
 * Copyright 2014 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STATE_H_
#define _STATE_H_

#include <stdbool.h>

#include <octoclock.h>

// Global state variables
extern volatile bool         g_ext_ref_present;
extern volatile bool         g_gps_present;
extern volatile switch_pos_t g_switch_pos;
extern volatile ref_t        g_ref;

typedef enum {
    LED_TOP,    // Internal
    LED_MIDDLE, // External
    LED_BOTTOM  // Status
} led_t;

void led(led_t which, bool on);

void leds_off(void);

void prefer_internal(void);

void prefer_external(void);

#endif /* _STATE_H_ */
