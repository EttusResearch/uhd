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

static void set_control_bit(int bit){
    set_control(1 << bit, 1 << bit);
}

static void clr_control_bit(int bit){
    set_control(0 << bit, 1 << bit);
}

static bool is_status_bit_set(int bit){
    return buffer_pool_status->status & (1 << bit);
}

#define INP_HS_BIT 0 //CPU out in packet_router.v
#define OUT_HS_BIT 1 //CPU inp in packet_router.v
#define MODE_BIT 2
#define CLR_BIT 8

void pkt_ctrl_register_ip_addr(const struct ip_addr *ip_addr){
    //program in the ip addr
    set_control(0x1 << 4, 0x3 << 4);
    set_control((ip_addr->addr & 0x0000ffff) << 16, 0xffff << 16);
    set_control(0x2 << 4, 0x3 << 4);
    set_control((ip_addr->addr & 0xffff0000) << 0,  0xffff << 16);

    //clear cmd
    set_control(0x0, 0x3 << 4);
}

void pkt_ctrl_set_routing_mode(pkt_ctrl_routing_mode_t mode){
    switch(mode){
    case PKT_CTRL_ROUTING_MODE_SLAVE:
        clr_control_bit(MODE_BIT);
        break;
    case PKT_CTRL_ROUTING_MODE_MASTER:
        set_control_bit(MODE_BIT);
        break;
    }
}

void *pkt_ctrl_claim_incoming_buffer(size_t *num_lines){
    if (!is_status_bit_set(INP_HS_BIT)) return NULL;
    *num_lines = (buffer_pool_status->status >> 16) & 0xffff;
    return buffer_ram(0);
}

void pkt_ctrl_release_incoming_buffer(void){
    set_control_bit(INP_HS_BIT);
    while (is_status_bit_set(INP_HS_BIT)){}
    clr_control_bit(INP_HS_BIT);
}

void *pkt_ctrl_claim_outgoing_buffer(void){
    while (!is_status_bit_set(OUT_HS_BIT)){}
    return buffer_ram(1);
}

void pkt_ctrl_commit_outgoing_buffer(size_t num_lines){
    set_control(num_lines << 16, 0xffff << 16);
    set_control_bit(OUT_HS_BIT);
    while (is_status_bit_set(OUT_HS_BIT)){}
    clr_control_bit(OUT_HS_BIT);
}
