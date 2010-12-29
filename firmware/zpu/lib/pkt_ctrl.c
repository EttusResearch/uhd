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
    const struct ip_addr *ip_addr, uint16_t data_port
){
    router_ctrl->ip_addr = ip_addr->addr;
    router_ctrl->data_ports = data_port;
}

void pkt_ctrl_set_routing_mode(pkt_ctrl_routing_mode_t mode){
    switch(mode){
    case PKT_CTRL_ROUTING_MODE_SLAVE:  router_ctrl->mode_ctrl = 0; break;
    case PKT_CTRL_ROUTING_MODE_MASTER: router_ctrl->mode_ctrl = 1; break;
    }
}

//status signals from WB into PR
#define CPU_STAT_RD_DONE (1 << 0)
#define CPU_STAT_RD_EROR (1 << 1)
#define CPU_STAT_RD_IDLE (1 << 2)

//status signals from PR into WB
#define CPU_STAT_WR_DONE (1 << 4)
#define CPU_STAT_WR_EROR (1 << 5)
#define CPU_STAT_WR_IDLE (1 << 6)

//control signals from WB into PR
#define CPU_CTRL_RD_CLEAR (1 << 0)
#define CPU_CTRL_RD_START (1 << 1)

//control signals from PR into WB
#define CPU_CTRL_WR_CLEAR (1 << 2)
#define CPU_CTRL_WR_START (1 << 3)

static bool i_am_writing;

static inline void cpu_stat_wait_for(int bm){
    while((router_status->status & bm) == 0){
        /* NOP */
    }
}

void pkt_ctrl_init(void){
    router_ctrl->iface_ctrl = CPU_CTRL_WR_CLEAR | CPU_CTRL_RD_START;
    i_am_writing = false;
}

void *pkt_ctrl_claim_incoming_buffer(size_t *num_lines){
    uint32_t status = router_status->status;

    //if done: clear the read and return the buffer
    if (status & CPU_STAT_RD_DONE){
        *num_lines = (router_status->status >> 16) & 0xffff;
        return ((uint32_t *) ROUTER_RAM_BASE);
    }

    //if error: drop the packet and start a new read
    else if (status & CPU_STAT_RD_EROR){
        putstr("E");
        pkt_ctrl_release_incoming_buffer();
    }

    //otherwise null for nothing ready
    return NULL;
}

void pkt_ctrl_release_incoming_buffer(void){
    //clear, wait for idle, and start a new read
    router_ctrl->iface_ctrl = CPU_CTRL_RD_CLEAR;
    cpu_stat_wait_for(CPU_STAT_RD_IDLE);
    router_ctrl->iface_ctrl = CPU_CTRL_RD_START;
}

void *pkt_ctrl_claim_outgoing_buffer(void){
    if (i_am_writing){
        //wait for the write to become done
        cpu_stat_wait_for(CPU_STAT_WR_DONE);
        router_ctrl->iface_ctrl = CPU_CTRL_WR_CLEAR;
        i_am_writing = false;
    }
    //wait for idle and return the buffer
    cpu_stat_wait_for(CPU_STAT_WR_IDLE);
    return ((uint32_t *) ROUTER_RAM_BASE);
}

void pkt_ctrl_commit_outgoing_buffer(size_t num_lines){
    //start a new write with the given length
    router_ctrl->iface_ctrl = ((num_lines & 0xffff) << 16) | CPU_CTRL_WR_START;
    i_am_writing = true;
}
