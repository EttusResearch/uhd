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


/* Changes required to work for the Spartan-3A series:
 * The ICAP interface on the 3A is 8 bits wide, instead of 32.
 * Everything is Xilinx standard LSBit-first.
 * The operations are all different.
 * Commands are 16 bits long, presented to the ICAP interface 8 bits at a time.
*/

#include <xilinx_s3_icap.h>
#include <memory_map.h>
#include <spi_flash_private.h> //for READ_CMD


/* bit swap end-for-end */
static inline unsigned char
swap8(unsigned char x)
{
  unsigned char r = 0;
  r |= (x >> 7) & 0x01;
  r |= (x >> 5) & 0x02;
  r |= (x >> 3) & 0x04;
  r |= (x >> 1) & 0x08;

  r |= (x << 1) & 0x10;
  r |= (x << 3) & 0x20;
  r |= (x << 5) & 0x40;
  r |= (x << 7) & 0x80;

  return r;
}

void
wr_icap(uint8_t x)
{
    icap_regs->icap = swap8(x);
}

uint8_t
rd_icap(void)
{
    return swap8(icap_regs->icap);
}


void
icap_reload_fpga(uint32_t flash_address)
{
    union {
        uint32_t i;
        uint8_t c[4];
    } t;
    t.i = flash_address;

    //note! t.c[0] MUST contain the byte-wide read command for the flash device used.
    //for the 25P64, and most other flash devices, this is 0x03.
    t.c[0] = FAST_READ_CMD;

    //TODO: look up the watchdog timer, ensure it won't fire too soon

    //UG332 p279
    wr_icap(0xff);
    wr_icap(0xff); //dummy word, probably unnecessary
    wr_icap(0xAA);
    wr_icap(0x99); //sync word
    wr_icap(0x32);
    wr_icap(0x61); //Type 1 write General 1 (1 word)
    wr_icap(t.c[2]); //bits 15-8
    wr_icap(t.c[3]); //bits 7-0
    wr_icap(0x32);
    wr_icap(0x81); //Type 1 write General 2 (1 word)
    wr_icap(t.c[0]); //C0-C8, the byte-wide read command
    wr_icap(t.c[1]); //Upper 8 bits of 24-bit address
    wr_icap(0x30);
    wr_icap(0xA1); //Type 1 write CMD (1 word)
    wr_icap(0x00);
    wr_icap(0x0E); //REBOOT command
    wr_icap(0x20);
    wr_icap(0x00); //Type 1 NOP
    wr_icap(0x20);
    wr_icap(0x00);
}
