/* -*- c -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

#include "mdelay.h"
#include "memory_map.h"

void mdelay(int ms){
  if (hwconfig_simulation_p()) return;
  for(int i = 0; i < ms; i++){
    static const uint32_t num_ticks = MASTER_CLK_RATE/1000;
    const uint32_t ticks_begin = router_status->time64_ticks_rb;
    while((router_status->time64_ticks_rb - ticks_begin) < num_ticks){
      /*NOP*/
    }
  }
}
