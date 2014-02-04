/* -*- c -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
 * Copyright 2009 Ettus Research LLC
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
#include "wb_utils.h"
#include "printf.h"
#include <stdint.h>
//IJB FIXME.
#include "../x300/x300_defs.h"

void mdelay(int ms){
  for(int i = 0; i < ms; i++){
    static const uint32_t num_ticks = CPU_CLOCK/1000;
    const uint32_t ticks_begin = wb_peek32(SR_ADDR(RB0_BASE, RB_COUNTER));
    //    printf("DEBUG: Counter is %d\n",ticks_begin);
    while((wb_peek32(SR_ADDR(RB0_BASE, RB_COUNTER)) - ticks_begin) < num_ticks) {
      /*NOP*/
    }
  }
}
