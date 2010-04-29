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
#include "buffer_pool.h"
#include "hal_uart.h"
#include "i2c.h"
#include "mdelay.h"
#include "clocks.h"
#include "usrp2_i2c_addr.h"

//#include "nonstdio.h"

unsigned char u2_hw_rev_major;
unsigned char u2_hw_rev_minor;

static inline void
get_hw_rev(void)
{
  bool ok = eeprom_read(I2C_ADDR_MBOARD, MBOARD_REV_LSB, &u2_hw_rev_minor, 1);
  ok &= eeprom_read(I2C_ADDR_MBOARD, MBOARD_REV_MSB, &u2_hw_rev_major, 1);
}

/*
 * We ought to arrange for this to be called before main, but for now,
 * we require that the user's main call u2_init as the first thing...
 */
bool
u2_init(void)
{
  hal_io_init();

  // init spi, so that we can switch over to the high-speed clock
  spi_init();

  // init i2c so we can read our rev
  i2c_init();
  get_hw_rev();

  // set up the default clocks
  clocks_init();

  pic_init();	// progammable interrupt controller
  bp_init();	// buffer pool
  
  hal_enable_ints();

  // flash all leds to let us know board is alive
  hal_set_leds(0x0, 0x1f);
  mdelay(100);
  hal_set_leds(0x1f, 0x1f);
  mdelay(100);
  hal_set_leds(0x1, 0x1f);  // Leave the first one on

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
  
  return true;
}
