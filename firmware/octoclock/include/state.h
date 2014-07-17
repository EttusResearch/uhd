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

// NOT PRESENT unless proven so...
static ref_t global_which_ref = NO_REF;
static bool global_gps_present = false;
static bool global_ext_ref_is_present = false;

volatile uint8_t ext_ref_buf[1024];

void led(LEDs which, int turn_it_on);

void LEDs_off(void);

void force_internal(void);

void prefer_internal(void);

void prefer_external(void);

bool is_ext_ref_present(void);

bool is_gps_present(void);

ref_t which_ref(void);

void check_what_is_present(void);

switch_pos_t get_switch_pos(void);

#endif /* _STATE_H_ */
