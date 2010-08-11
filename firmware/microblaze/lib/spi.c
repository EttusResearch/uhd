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

#include "spi.h"
#include "memory_map.h"
#include "pic.h"
#include "nonstdio.h"

void (*volatile spi_callback)(void); //SPI callback when xfer complete.

void
spi_init(void) 
{
  /*
   * f_sclk = f_wb / ((div + 1) * 2)
   */
  spi_regs->div = 1;  // 0 = Div by 2 (25 MHz); 1 = Div-by-4 (12.5 MHz)
}

void
spi_wait(void) 
{
  while (spi_regs->ctrl & SPI_CTRL_GO_BSY)
    ;
}

uint32_t
spi_transact(bool readback, int slave, uint32_t data, int length, uint32_t flags) 
{
  flags &= (SPI_CTRL_TXNEG | SPI_CTRL_RXNEG);
  int ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & length) | flags;

  spi_wait();

  // Tell it which SPI slave device to access
  spi_regs->ss = slave & 0xffff;

  // Data we will send
  spi_regs->txrx0 = data;

  // Run it -- write once and rewrite with GO set
  spi_regs->ctrl = ctrl;
  spi_regs->ctrl = ctrl | SPI_CTRL_GO_BSY;

  if(readback) {
    spi_wait();
    return spi_regs->txrx0;
  }
  else
    return 0;
}

void spi_register_callback(void (*volatile callback)(void)) {
  spi_callback = callback;
}

void spi_irq_handler(void) {
  printf("SPI IRQ handler\n");
  if(spi_callback) spi_callback(); //we could just use the PIC to register the user's callback, but this provides the ability to do other things later
}

uint32_t spi_get_data(void) {
  return spi_regs->txrx0;
}

bool 
spi_async_transact(int slave, uint32_t data, int length, uint32_t flags, void (*volatile callback)(void)) {
  flags &= (SPI_CTRL_TXNEG | SPI_CTRL_RXNEG);
  int ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & length) | flags;

  if(spi_regs->ctrl & SPI_CTRL_GO_BSY) return false; //we don't wait on busy, we just return failure.

  // Tell it which SPI slave device to access
  spi_regs->ss = slave & 0xffff;

  // Data we will send
  spi_regs->txrx0 = data;

  // Run it -- write once and rewrite with GO set
  spi_regs->ctrl = ctrl;
  spi_regs->ctrl = ctrl | SPI_CTRL_GO_BSY;

  spi_regs->ctrl |= SPI_CTRL_IE; //we do these here so that we don't have to start the PIC before the SPI sets up the clocks on startup
  pic_register_handler(IRQ_SPI, spi_irq_handler);

  spi_register_callback(callback);

  return true;
}
