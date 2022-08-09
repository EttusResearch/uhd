/*
 * Copyright 2009 Free Software Foundation, Inc.
 * Copyright 2009-2012 Ettus Research LLC
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

#include "spi_flash.h"
#include "spi_flash_private.h"
//#include <stdlib.h>
#include <nonstdio.h>

uint32_t
spi_flash_rdsr(void)
{
  return spif_transact(SPI_TXRX, SPI_SS_FLASH, RDSR_CMD << 8, 16, FLAGS) & 0xff;
}

static void
spi_flash_write_enable(void)
{
//	spif_transact(SPI_TXONLY, SPI_SS_FLASH, (WRSR_CMD << 8) | 0x00, 16, FLAGS); //disable write protection bits
  spif_transact(SPI_TXONLY, SPI_SS_FLASH, WREN_CMD, 8, FLAGS);
}

bool
spi_flash_done_p(void)
{
  return (spi_flash_rdsr() & SR_WIP) == 0;
}

void
spi_flash_wait(void)
{
  while (!spi_flash_done_p())
    ;
}

void
spi_flash_erase_sector_start(uint32_t flash_addr)
{
  //printf("spi_flash_erase_sector_start: addr = 0x%x\n", flash_addr);
  if(flash_addr > spi_flash_memory_size())
    return;

  spi_flash_wait();
  spi_flash_write_enable();
  spif_transact(SPI_TXONLY, SPI_SS_FLASH,
		(SE_CMD << 24) | (flash_addr & 0x00ffffff),
		32, FLAGS);
}

bool
spi_flash_page_program_start(uint32_t flash_addr, size_t nbytes, const void *buf)
{
  if (nbytes == 0 || nbytes > SPI_FLASH_PAGE_SIZE)
    return false;

  //please to not be writing past the end of the device
  if ((flash_addr + nbytes) > spi_flash_memory_size())
    return false;

  uint32_t local_buf[SPI_FLASH_PAGE_SIZE / sizeof(uint32_t)];
  memset(local_buf, 0xff, sizeof(local_buf));	// init to 0xff (nops when programming)
  memcpy(local_buf, buf, nbytes);

  spi_flash_wait();
  spi_flash_write_enable();
  
  /*
   * We explicitly control the slave select here (/S), so that we can
   * do the entire write operation as a single transaction from
   * device's point of view.  (The most our SPI peripheral can transfer
   * in a single shot is 16 bytes.)
   */
  spif_wait();

  spif_regs->ss = 0;
  spif_regs->ctrl = FLAGS;	// ASS is now clear and no chip select is enabled.
  
  /* write PP_CMD, ADDR2, ADDR1, ADDR0 */

  spif_regs->txrx0 = (PP_CMD << 24) | (flash_addr & 0x00ffffff);
  spif_regs->ss = SPI_SS_FLASH;		// assert chip select
  spif_regs->ctrl = FLAGS | LEN(4 * 8);		
  spif_regs->ctrl = FLAGS | LEN(4 * 8) | SPI_CTRL_GO_BSY;
  spif_wait();

  /*  send 256 bytes total, 16 at a time */
  for (size_t i = 0; i < 16; i++){
    spif_regs->txrx3 = local_buf[i * 4 + 0];
    spif_regs->txrx2 = local_buf[i * 4 + 1];
    spif_regs->txrx1 = local_buf[i * 4 + 2];
    spif_regs->txrx0 = local_buf[i * 4 + 3];
    
    spif_regs->ctrl = FLAGS | LEN(16 * 8);	// xfer 16 bytes
    spif_regs->ctrl = FLAGS | LEN(16 * 8) | SPI_CTRL_GO_BSY;
    spif_wait();
  }
  spif_regs->ss = 0;		// desassert chip select

  return true;
}

void
spi_flash_erase(uint32_t flash_addr, size_t nbytes)
{
  if (nbytes == 0)
    return;

  uint32_t first = round_down(flash_addr, spi_flash_sector_size());
  uint32_t last  = round_down(flash_addr + nbytes - 1, spi_flash_sector_size());
  
  for (uint32_t s = first; s <= last; s += spi_flash_sector_size()){
    spi_flash_erase_sector_start(s);
  }
  spi_flash_wait();
}

bool
spi_flash_program(uint32_t flash_addr, size_t nbytes, const void *buf)
{
  //uprintf(UART_DEBUG, "\nspi_flash_program: addr = 0x%x, nbytes = %d\n", flash_addr, nbytes);

  const unsigned char *p = (const unsigned char *) buf;
  size_t n;

  if ((nbytes + flash_addr) > spi_flash_memory_size())
    return false;
  if (nbytes == 0)
    return true;

  uint32_t r = flash_addr % SPI_FLASH_PAGE_SIZE;
  if (r){	/* do initial non-aligned page */
    n = min(SPI_FLASH_PAGE_SIZE - r, nbytes);
    spi_flash_page_program_start(flash_addr, n, p);
    flash_addr += n;
    p += n;
    nbytes -= n;
  }

  while (nbytes > 0){
    n = min(SPI_FLASH_PAGE_SIZE, nbytes);
    spi_flash_page_program_start(flash_addr, n, p);
    flash_addr += n;
    p += n;
    nbytes -= n;
  }

  spi_flash_wait();
  return true;
}

void
spi_flash_async_erase_start(spi_flash_async_state_t *s,
			    uint32_t flash_addr, size_t nbytes)
{

  //printf("got command to erase %d bytes at 0x%x\n", nbytes, flash_addr);
  
  if ((nbytes == 0) || ((flash_addr + nbytes) > spi_flash_memory_size())){
    s->first = s->last = s->current = 0;
    return;
  }

  uint32_t first = round_down(flash_addr, spi_flash_sector_size());
  uint32_t last  = round_down(flash_addr + nbytes - 1, spi_flash_sector_size());

  s->first = first;
  s->last = last;
  s->current = first;

  spi_flash_erase_sector_start(s->current);
}

bool
spi_flash_async_erase_poll(spi_flash_async_state_t *s)
{
  if (!spi_flash_done_p())
    return false;

  //printf("%d/%d\n", s->current, s->last);

  // The current sector erase has completed.  See if we're finished or
  // if there's more to do.

  if (s->current == s->last)	// we're done!
    return true;

  s->current += spi_flash_sector_size();
  spi_flash_erase_sector_start(s->current);
  return false;
}

