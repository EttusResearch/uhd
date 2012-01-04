/* -*- c -*- */
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#include "memory_map.h"
#include "hal_uart.h"
#include "hal_io.h"
#include "mdelay.h"

//just to save you from going insane, note that firmware/FPGA UARTs [0-2] correspond to serial ports [1-3].
//so in software, we refer to UART_DEBUG as UART0, but it transmits on pin TXD<1>. see the UART assignments in hal_uart.h.

#define NSPEEDS 6
#define	MAX_WB_DIV 4

//if you're going to recalculate the divisors, it's just uart_clock_rate / baud_rate.
//uart_clock_rate is 50MHz for USRP2.
static const uint16_t
divisor_table[NSPEEDS] = {
  5208,	//    9600
  2604,	//   19200
  1302,	//   38400
  868,	//   57600
  434,	//  115200
  217	//  230400
};

static char uart_mode[4] = {
  [UART_DEBUG] = UART_MODE_ONLCR, 
  [UART_EXP] = UART_MODE_ONLCR, 
  [UART_GPS] = UART_MODE_ONLCR
};

static char uart_speeds[4] = {
  [UART_DEBUG] = US_230400,
  [UART_EXP] = US_230400,
  [UART_GPS] = US_115200
};

void
hal_uart_set_mode(hal_uart_name_t uart, int mode)
{
  uart_mode[uart] = mode;
}

void hal_uart_set_speed(hal_uart_name_t uart, hal_uart_speed_t speed)
{
  uart_regs[uart].clkdiv = divisor_table[speed];
}

void
hal_uart_init(void)
{
  for(int i = 0; i < 3; i++) {
  	hal_uart_set_mode(i, uart_mode[i]);
    hal_uart_set_speed(i, uart_speeds[i]);
  }
}

void
hal_uart_putc(hal_uart_name_t u, int ch)
{
  if ((ch == '\n') && (uart_mode[u] == UART_MODE_ONLCR))		//map \n->\r\n if necessary
    hal_uart_putc(u, '\r');

  while (uart_regs[u].txlevel == 0)	 // wait for fifo to have space
    ;

  uart_regs[u].txchar = ch;
}

void
hal_uart_putc_nowait(hal_uart_name_t u, int ch)
{
  if ((ch == '\n') && (uart_mode[u] == UART_MODE_ONLCR))		//map \n->\r\n if necessary
    hal_uart_putc(u, '\r');

  if(uart_regs[u].txlevel)   // If fifo has space
    uart_regs[u].txchar = ch;
}

int
hal_uart_getc(hal_uart_name_t u)
{
  while ((uart_regs[u].rxlevel) == 0)  // wait for data to be ready
    ;

  return uart_regs[u].rxchar;
}

int 
hal_uart_getc_noblock(hal_uart_name_t u)
{
//  int timeout = 0;
//  while (((uart_regs[u].rxlevel) == 0) && (timeout++ < HAL_UART_TIMEOUT_MS))
//    mdelay(1);
  if(uart_regs[u].rxlevel == 0) return -1;
  return uart_regs[u].rxchar;
}

int hal_uart_rx_flush(hal_uart_name_t u)
{
  char x = 0;
  while(uart_regs[u].rxlevel) x = uart_regs[u].rxchar;
  return x;
}

