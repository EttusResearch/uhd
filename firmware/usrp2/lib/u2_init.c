/*
 * Copyright 2007 Free Software Foundation, Inc.
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

#include "u2_init.h"
#include "memory_map.h"
#include "spi.h"
#include "pic.h"
#include "hal_io.h"
#include "hal_uart.h"
#include "i2c.h"
#include "mdelay.h"
#include "clocks.h"
#include "usrp2/fw_common.h"
#include "nonstdio.h"

/*
 * We ought to arrange for this to be called before main, but for now,
 * we require that the user's main call u2_init as the first thing...
 */
bool
u2_init(void)
{
  hal_disable_ints();
  hal_io_init();

  // init spi, so that we can switch over to the high-speed clock
  spi_init();

  // set up the default clocks
  clocks_init();

  hal_uart_init();

  // init i2c so we can read our rev
  pic_init();	// progammable interrupt controller
  i2c_init();
  hal_enable_ints();

  // flash all leds to let us know board is alive
#ifndef BOOTLOADER
  hal_set_led_src(0x0, 0x1f); /* software ctrl */
  hal_set_leds(0x0, 0x1f);    mdelay(300);
  hal_set_leds(LED_E, LED_E); mdelay(300);
  hal_set_leds(LED_C, LED_C); mdelay(300);
  hal_set_leds(LED_A, LED_A); mdelay(300);
  for (int i = 0; i < 3; i++){ //blink all
    static const int blinks = LED_E | LED_C | LED_A;
    hal_set_leds(0x0,    0x1f); mdelay(100);
    hal_set_leds(blinks, 0x1f); mdelay(100);
  }
#endif
  hal_set_led_src(0x1f & ~LED_D, 0x1f); /* hardware ctrl */
  hal_set_leds(LED_D, 0x1f);  // Leave one on

#if 0
  // test register readback
  int rr, vv;
  vv = ad9777_read_reg(0);
  printf("ad9777 reg[0] = 0x%x\n", vv);
  
  for (rr = 0x04; rr <= 0x0d; rr++){
    vv = ad9510_read_reg(rr);
    printf("ad9510 reg[0x%x] = 0x%x\n", rr, vv);
  }
#endif

  output_regs->serdes_ctrl = (SERDES_ENABLE | SERDES_RXEN);

  return true;
}
