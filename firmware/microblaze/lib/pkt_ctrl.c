/*
 * Copyright 2010 Ettus Research LLC
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

#include "pkt_ctrl.h"
#include "memory_map.h"
#include <stdint.h>
#include <stdbool.h>
#include <nonstdio.h>

static void set_control(uint32_t value, uint32_t mask){
    static uint32_t ctrl_shadow = 0;

    ctrl_shadow &= ~mask;
    ctrl_shadow |= value & mask;

    buffer_pool_ctrl->ctrl = ctrl_shadow;
}

void *claim_incoming_buffer(size_t *num_lines){
    if ((buffer_pool_status->status & 0x1) != 1) return NULL;
    *num_lines = (buffer_pool_status->status >> 16) & 0xffff;
    set_control(0x1, 0x1);
    return buffer_ram(0);
}

void release_incoming_buffer(void){
    set_control(0x0, 0x1);
    while ((buffer_pool_status->status & 0x1) != 0){}
}

void *claim_outgoing_buffer(void){
    while ((buffer_pool_status->status & 0x2) != 0x2){}
    return buffer_ram(1);
}

void commit_outgoing_buffer(size_t num_lines){
    set_control(0x2 | (num_lines << 16), 0x2 | (0xffff << 16));
    while ((buffer_pool_status->status & 0x2) != 0x0){}
    set_control(0x0, 0x2);
}
