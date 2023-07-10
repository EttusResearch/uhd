/*
 * Copyright 2011 Ettus Research LLC
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

#include "udp_uart.h"
#include "hal_uart.h"
#include "net_common.h"
#include "compiler.h"
#include <stdbool.h>

/***********************************************************************
 * Constants
 **********************************************************************/
#define MAX_NUM_UARTS 4
#ifndef UDP_UART_MASK
    #error missing definition for UDP_UART_MASK enable mask
#endif
static const size_t num_idle_cyc_b4_flush = 11; //small but lucky number

/***********************************************************************
 * Globals
 **********************************************************************/
static uint16_t _base_port;

typedef struct{
    struct socket_address dst;
    _AL4 uint8_t buf[256];
    size_t len; //length of buffer
    size_t cyc; //idle cycle count
} udp_uart_state_t;

static udp_uart_state_t _states[MAX_NUM_UARTS];

/***********************************************************************
 * UDP handler for UARTs
 **********************************************************************/
static void handle_uart_data_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    //handle ICMP destination unreachable
    if (payload == NULL){
        const size_t which = src.port-_base_port;
        if (which >= MAX_NUM_UARTS) return;
        _states[which].dst.port = 0;
    }

    //handle a regular blocking UART write
    else{
        const size_t which = dst.port-_base_port;
        if (which >= MAX_NUM_UARTS) return;
        _states[which].dst = src;
        for (size_t i = 0; i < payload_len; i++){
            hal_uart_putc((hal_uart_name_t)which, (int)payload[i]);
        }
    }
}

/***********************************************************************
 * Public init function
 **********************************************************************/
void udp_uart_init(const uint16_t base_port){
    _base_port = base_port;
    for(size_t i = 0; i < MAX_NUM_UARTS; i++){
        _states[i].dst.port = 0; //reset to null port
        _states[i].len = 0;
        _states[i].cyc = 0;
        register_udp_listener(_base_port+i, handle_uart_data_packet);
    }
}

/***********************************************************************
 * Public poll function
 **********************************************************************/
void udp_uart_poll(void){
    for (size_t i = 0; i < MAX_NUM_UARTS; i++){
        if (((UDP_UART_MASK) & (1 << i)) == 0) continue;

        bool newline = false;
        udp_uart_state_t *state = &_states[i];

        //read all characters we can without blocking
        for (size_t j = state->len; j < sizeof(_states[0].buf); j++){
            int ret = hal_uart_getc_noblock((hal_uart_name_t)i);
            if (ret == -1) break;
            char ch = (char) ret;
            if (ch == '\n' || ch == '\r') newline = true;
            state->buf[j] = ch;
            state->len++;
            state->cyc = 0; //reset idle cycles
        }

        //nothing in buffer, continue to next uart
        if (state->len == 0) continue;

        //send out a message if newline or forced flush
        if (newline || state->cyc++ > num_idle_cyc_b4_flush){
            if (state->dst.port != 0) send_udp_pkt(_base_port+i, state->dst, state->buf, state->len);
            state->len = 0;
            state->cyc = 0;
        }
    }
}
