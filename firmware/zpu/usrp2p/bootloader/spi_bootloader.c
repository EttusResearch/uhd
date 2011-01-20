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

#include <hal_io.h>
#include <nonstdio.h>
#include <mdelay.h>
#include <spi_flash.h>
#include <quadradio/flashdir.h>
#include <quadradio/simple_binary_format.h>
#include <stdlib.h>


void hal_uart_init(void);
void spif_init(void);

void pic_interrupt_handler() __attribute__ ((interrupt_handler));

void pic_interrupt_handler()
{
  // nop stub
}

static void
error(int e)
{
  putstr("ERR");
  puthex8(e);
  newline();
}

static void
load(uint32_t flash_addr, uint32_t ram_addr, uint32_t size)
{
  spi_flash_read(flash_addr, size, (void *) ram_addr);
}

static bool
load_from_slot(const struct flashdir *fd, int fw_slot)
{
    putstr("Loading f/w image ");
    putchar('0' + fw_slot);
    putstr("... ");

    if (fw_slot >= fd->fw_nslots){
      error(1);
      return false;
    }

    int slot = fw_slot + fd->fw_slot0;
    if (fd->slot[slot].start == 0 || fd->slot[slot].start == 0xffff
	|| fd->slot[slot].len == 0 || fd->slot[slot].len == 0xffff){
      error(2);
      return false;
    }
    
    uint32_t sbf_base = fd->slot[slot].start << spi_flash_log2_sector_size();
    uint32_t sbf_len  = fd->slot[slot].len << spi_flash_log2_sector_size();
    uint32_t sbf_offset = 0;

    struct sbf_header sbf;
    spi_flash_read(sbf_base, sizeof(struct sbf_header), &sbf);
    if (sbf.magic != SBF_MAGIC || sbf.nsections > SBF_MAX_SECTIONS){
      error(3);
      return false;
    }
    sbf_offset += sizeof(struct sbf_header);

    unsigned int i;
    for (i = 0; i < sbf.nsections; i++){
      if (sbf_offset + sbf.sec_desc[i].length > sbf_len){
	error(4);
	return false;
      }
      load(sbf_offset + sbf_base,
	   sbf.sec_desc[i].target_addr,
	   sbf.sec_desc[i].length);
      sbf_offset += sbf.sec_desc[i].length;
    }
    putstr("Done!");

    typedef void (*fptr_t)(void);
    (*(fptr_t) sbf.entry)();		// almost certainly no return

    return true;
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

  putstr("\n>>> spi_bootloader <<<\n");

  const struct flashdir *fd = get_flashdir();
  if (fd == 0)
    abort();

  while(1){
    int sw;
    int fw_slot;
    
    sw = readback->switches;
    fw_slot = sw & 0x7;

    if (!load_from_slot(fd, fw_slot)){
      if (fw_slot != 0){
	putstr("Falling back to slot 0\n");
	load_from_slot(fd, 0);
      }
    }
  }
}
