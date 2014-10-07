/* -*- c++ -*- */
/*
 * Copyright 2009 Free Software Foundation, Inc.
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

#include "spi_flash_private.h"
#include <stdlib.h>		// abort
#include <nonstdio.h>

uint32_t 
spi_flash_rdid(void)
{
  return spif_transact(SPI_TXRX, SPI_SS_FLASH, RDID_CMD << 24, 32, FLAGS) & 0xffffff;
}

size_t spi_flash_log2_memory_size(void)
{
    static size_t _spi_flash_log2_memory_size = 0;
    if (_spi_flash_log2_memory_size == 0){
        uint32_t id = spi_flash_rdid();
        uint8_t type = (id >> 8) & 0xff;
        uint8_t size = (id >> 0) & 0xff;
        if (type != 0x20) abort();
        _spi_flash_log2_memory_size = size;
    }
    if (_spi_flash_log2_memory_size < 22 ||
        _spi_flash_log2_memory_size > 24 ) abort();
    return _spi_flash_log2_memory_size;
}

size_t spi_flash_log2_sector_size(void)
{
    static unsigned char log2_sector_size[3] = {
        16, /* M25P32  */
        16, /* M25P64  */
        18, /* M25P128 */
    };
    return log2_sector_size[spi_flash_log2_memory_size() - 22];
}

size_t spi_flash_sector_size(void)
{
  return ((size_t) 1) << spi_flash_log2_sector_size();
}

size_t spi_flash_memory_size(void)
{
  return ((size_t) 1) << spi_flash_log2_memory_size();
}

void 
spi_flash_read(uint32_t flash_addr,  size_t nbytes, void *buf)
{
  /*
   * We explicitly control the slave select here (/S), so that we can
   * do the entire read operation as a single transaction from
   * device's point of view.  (The most our SPI peripheral can transfer
   * in a single shot is 16 bytes.)
   */
  spif_wait();

  spif_regs->ss = 0;
  spif_regs->ctrl = FLAGS;	// ASS is now clear and no chip select is enabled.
  
  /*
   * Do the 5 byte instruction tranfer:
   *   FAST_READ_CMD, ADDR2, ADDR1, ADDR0, DUMMY
   */
  spif_regs->txrx1 = FAST_READ_CMD;
  spif_regs->txrx0 = ((flash_addr & 0x00ffffff) << 8);
  spif_regs->ss = SPI_SS_FLASH;		// assert chip select
  spif_regs->ctrl = FLAGS | LEN(5 * 8);		
  spif_regs->ctrl = FLAGS | LEN(5 * 8) | SPI_CTRL_GO_BSY;
  spif_wait();

  /*
   * Read up to 16 bytes at a time until done
   */
  unsigned char *dst = (unsigned char *) buf;
  size_t m;
  for (size_t n = 0; n < nbytes; n += m){
        
    spif_regs->ctrl = FLAGS | LEN(16 * 8);	// xfer 16 bytes
    spif_regs->ctrl = FLAGS | LEN(16 * 8) | SPI_CTRL_GO_BSY;
    spif_wait();

    uint32_t w[4];
    w[0] = spif_regs->txrx3;	// txrx3 has first bits in it
    w[1] = spif_regs->txrx2;
    w[2] = spif_regs->txrx1;
    w[3] = spif_regs->txrx0;
    unsigned char *src = (unsigned char *) &w[0];
    m = min(nbytes - n, 16);
    for (size_t i = 0; i < m; i++)
      *(dst++) = src[i];
  }
  spif_regs->ss = 0;			// deassert chip select
}
