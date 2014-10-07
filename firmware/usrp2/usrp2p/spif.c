/*
 * Copyright 2007,2008,2009 Free Software Foundation, Inc.
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

/*
 * Code for the Flash SPI bus
 */

#include "spi.h"
#include "spi_flash.h"
#include "memory_map.h"

void
spif_init(void) 
{
  /*
   * f_sclk = f_wb / ((div + 1) * 2)
   */
  spif_regs->div = 1;  // 0 = Div by 2 (31.25 MHz); 1 = Div-by-4 (15.625 MHz)

  // run dummy transaction to work around invalid initial clock state
  spif_transact(SPI_TXONLY, 0, 0, 8, SPIF_PUSH_FALL | SPIF_LATCH_RISE);
}

inline void
spif_wait(void) 
{
  while (spif_regs->ctrl & SPI_CTRL_GO_BSY)
    ;
}

uint32_t
spif_transact(bool readback_, int slave, uint32_t data, int length, uint32_t flags) 
{
  flags &= (SPI_CTRL_TXNEG | SPI_CTRL_RXNEG);
  int ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & length) | flags;

  spif_wait();

  // Data we will send
  spif_regs->txrx0 = data;

  // Run it -- write once and rewrite with GO set
  spif_regs->ctrl = ctrl;
  // Tell it which SPI slave device to access
  spif_regs->ss = slave & 0xff;
  spif_regs->ctrl = ctrl | SPI_CTRL_GO_BSY;

  if(readback_) {
    spif_wait();
    return spif_regs->txrx0;
  }
  else
    return 0;
}
