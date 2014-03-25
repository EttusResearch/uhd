/*
 * Copyright 2011-2013 Ettus Research LLC
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

#include <wb_uart.h>
#include <udp_uart.h>
#include <u3_net_stack.h>

/***********************************************************************
 * Constants
 **********************************************************************/
#define MAX_NUM_UARTS 4

static const size_t num_idle_cyc_b4_flush = 22;

/***********************************************************************
 * Globals
 **********************************************************************/
typedef struct
{
    uint32_t uart_base;
    struct ip_addr host_addr;
    int host_ethno;
    uint16_t host_port;
    uint16_t local_port;
    __attribute__ ((aligned (16))) uint8_t buf[256];
    size_t len; //length of buffer
    size_t cyc; //idle cycle count
} udp_uart_state_t;

static udp_uart_state_t _states[MAX_NUM_UARTS];

static int udp_uart_lookup(const uint16_t port)
{
    for (size_t i = 0; i < MAX_NUM_UARTS; i++)
    {
        if (_states[i].local_port == port) return i;
    }
    return -1;
}

/***********************************************************************
 * UDP handler for UARTs
 **********************************************************************/
static void handle_uart_data_packet(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
){
    //handle ICMP destination unreachable
    if (buff == NULL)
    {
        const size_t which = udp_uart_lookup(src_port);
        if (which == -1) return;
        _states[which].host_port = 0;
    }

    //handle a regular blocking UART write
    else
    {
        const size_t which = udp_uart_lookup(dst_port);
        if (which == -1) return;
        _states[which].host_ethno = ethno;
        _states[which].host_addr = *src;
        _states[which].host_port = src_port;
        for (size_t i = 0; i < num_bytes; i++)
        {
            const char ch = ((const char *)buff)[i];
            if (ch == '\n') wb_uart_putc(_states[which].uart_base, (int)'\r');
            wb_uart_putc(_states[which].uart_base, (int)ch);
            udp_uart_poll();
        }
    }
}

/***********************************************************************
 * Public init function
 **********************************************************************/
void udp_uart_init(const uint32_t uart_base, const uint16_t udp_port)
{
    for (size_t i = 0; i < MAX_NUM_UARTS; i++)
    {
        if (_states[i].uart_base != 0) continue;
        _states[i].uart_base = uart_base;
        _states[i].local_port = udp_port;
        _states[i].host_port = 0; //reset to null port
        _states[i].len = 0;
        _states[i].cyc = 0;
        u3_net_stack_register_udp_handler(udp_port, &handle_uart_data_packet);
        return;
    }
}

/***********************************************************************
 * Public poll function
 **********************************************************************/
void udp_uart_poll(void)
{
    for (size_t i = 0; i < MAX_NUM_UARTS; i++)
    {
        if (_states[i].uart_base == 0) continue;

        bool newline = false;
        udp_uart_state_t *state = &_states[i];

        //read all characters we can without blocking
        for (size_t j = state->len; j < sizeof(state->buf); j++)
        {
            int ret = wb_uart_getc(state->uart_base);
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
        if (newline || state->cyc++ > num_idle_cyc_b4_flush)
        {
            if (state->host_port != 0) u3_net_stack_send_udp_pkt(
                state->host_ethno,
                &state->host_addr,
                state->local_port,
                state->host_port,
                state->buf, state->len
            );
            state->len = 0;
            state->cyc = 0;
        }
    }
}
