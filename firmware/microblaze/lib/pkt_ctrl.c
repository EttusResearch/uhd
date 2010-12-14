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
#include <nonstdio.h>

void pkt_ctrl_program_inspector(
    const struct ip_addr *ip_addr, uint16_t ctrl_port, uint16_t data_port
){
    buffer_pool_ctrl->ip_addr = ip_addr->addr;
    buffer_pool_ctrl->ctrl_ports = ctrl_port;
    buffer_pool_ctrl->data_ports = data_port;
}

void pkt_ctrl_set_routing_mode(pkt_ctrl_routing_mode_t mode){
    switch(mode){
    case PKT_CTRL_ROUTING_MODE_SLAVE:
        buffer_pool_ctrl->misc_ctrl = 0;
        break;
    case PKT_CTRL_ROUTING_MODE_MASTER:
        buffer_pool_ctrl->misc_ctrl = 1;
        break;
    }
}

static inline bool is_status_bit_set(int bit){
    return buffer_pool_status->status & (1 << bit);
}

#define CPU_OUT_HS_BIT 0 //from packet router to CPU
#define CPU_INP_HS_BIT 1 //from CPU to packet router

void *pkt_ctrl_claim_incoming_buffer(size_t *num_lines){
    if (!is_status_bit_set(CPU_OUT_HS_BIT)) return NULL;
    *num_lines = (buffer_pool_status->status >> 16) & 0xffff;
    return buffer_ram(0);
}

void pkt_ctrl_release_incoming_buffer(void){
    buffer_pool_ctrl->cpu_out_ctrl = 1;
    while (is_status_bit_set(CPU_OUT_HS_BIT)){}
    buffer_pool_ctrl->cpu_out_ctrl = 0;
}

void *pkt_ctrl_claim_outgoing_buffer(void){
    while (!is_status_bit_set(CPU_INP_HS_BIT)){}
    return buffer_ram(1);
}

void pkt_ctrl_commit_outgoing_buffer(size_t num_lines){
    buffer_pool_ctrl->cpu_inp_ctrl = ((num_lines & 0xffff) << 16) | 1;
    while (is_status_bit_set(CPU_INP_HS_BIT)){}
    buffer_pool_ctrl->cpu_inp_ctrl = 0;
}
