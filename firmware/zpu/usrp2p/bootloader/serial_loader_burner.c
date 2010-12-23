/* -*- c++ -*- */
/*
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

#include <hal_io.h>
#include <nonstdio.h>
#include <mdelay.h>
#include <gdbstub2.h>

void hal_uart_init(void);
void spif_init(void);

void pic_interrupt_handler() __attribute__ ((interrupt_handler));

void pic_interrupt_handler()
{
  // nop stub
}

int
main(int argc, char **argv)
{
  hal_uart_init();
  spif_init();

  sr_leds->leds =  0;
  mdelay(100);
  sr_leds->leds = ~0;
  mdelay(100);
  sr_leds->leds =  0;

  puts("\n\n>>> stage1: serial_loader_burner <<<");

  gdbstub2_main_loop();
}
