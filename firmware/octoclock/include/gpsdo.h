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

#ifndef _GPSDO_H_
#define _GPSDO_H_

#include <stdint.h>

#define POOLSIZE 256

char gpsdo_buf[POOLSIZE];
gpsdo_cache_state_t gpsdo_state;

void send_gpsdo_cmd(char* buf, uint8_t size);

#endif /* _GPSDO_H_ */
