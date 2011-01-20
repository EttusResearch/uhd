/* -*- c -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
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

#include <memory_map.h>
#include <nonstdio.h>
#include <stdlib.h>
#include <bootconfig.h>
#include <bootconfig_private.h>
#include <bootloader_utils.h>
#include <hal_interrupts.h>


void hal_uart_init(void);
void spif_init(void);
void i2c_init(void);
void bootconfig_init(void);

void pic_interrupt_handler() __attribute__ ((interrupt_handler));

void pic_interrupt_handler()
{
  // nop stub
}

int
main(int argc, char **argv)
{
  hal_disable_ints();	// In case we got here via jmp 0x0
  hal_uart_init();
  i2c_init();
  bootconfig_init();	// Must come after i2c_init.
  spif_init();		// Needed for get_flashdir.

  load_firmware();
}
