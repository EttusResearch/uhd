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

#include "loader_parser.h"
#include <quadradio/loader_bits.h>
#include <quadradio/flashdir.h>
#include <quadradio/simple_binary_format.h>
#include <spi_flash.h>
#include <nonstdio.h>
//#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "ethernet.h"
#include "qr_settings.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

static spi_flash_async_state_t async_state;


static caldiv_eeprom_setter_t _caldiv_set_rev = NULL;
static caldiv_eeprom_setter_t _caldiv_set_ser = NULL;
static caldiv_eeprom_setter_t _caldiv_set_mod = NULL;

void
register_caldiv_eeprom_setters(caldiv_eeprom_setter_t set_rev,
			       caldiv_eeprom_setter_t set_ser,
			       caldiv_eeprom_setter_t set_mod)
{
  _caldiv_set_rev = set_rev;
  _caldiv_set_ser = set_ser;
  _caldiv_set_mod = set_mod;
}


// big-endian
static uint32_t 
get32(const unsigned char *s)
{
  return (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
}

// big-endian
static unsigned char *
put32(unsigned char *s, uint32_t v)
{
  s[0] = (v >> 24) & 0xff;
  s[1] = (v >> 16) & 0xff;
  s[2] = (v >>  8) & 0xff;
  s[3] = v & 0xff;
  return s + 4;
}

static bool
erased_p(uint32_t flash_addr, size_t nbytes)
{
  unsigned char	buf[64];

  size_t n;
  for (size_t i = 0; i < nbytes; i += n, flash_addr += n){
    n = min(nbytes - i, sizeof(buf));
    spi_flash_read(flash_addr, n, buf);
    for (size_t j = 0; j < n; j++)
      if (buf[j] != 0xff)
	return false;
  }
  return true;
}

static bool
erase_flash(uint32_t addr, uint32_t len)
{
  if (addr % spi_flash_sector_size() != 0)
    return false;

  if (len % spi_flash_sector_size() != 0)
    return false;

  spi_flash_async_erase_start(&async_state, addr, len);
  // FIXME? check to see if erase was successful
  return true;
}

static bool
map_slot(uint32_t slot, uint32_t *slot_start, uint32_t *slot_len, uint32_t *status)
{
  // This case doesn't require a valid flashdir, and in fact can be used as 
  // part of writing the intial flashdir.
  if (QLD_SLOT_DOM(slot) == QLD_DOM_UNMAPPED){
    int flash_size = get_flash_size();
    if (flash_size == 0){
      *status = QLDS_FAILED;	// Can't find the flash.  most likely a h/w problem.
      return false;
    }
    *slot_start = 0;
    *slot_len = flash_size;
    return true;
  }

  const struct flashdir *fd = get_flashdir();
  if (fd == 0)
    return false;

  uint32_t slot_num = QLD_SLOT_NUM(slot);

  switch(QLD_SLOT_DOM(slot)){
  case QLD_DOM_FPGA:
    if (slot_num >= fd->fpga_nslots){
      *status = QLDS_INVALID_ARG;
      return false;
    }
    *slot_start = fd->slot[slot_num + fd->fpga_slot0].start << spi_flash_log2_sector_size();
    *slot_len = fd->slot[slot_num + fd->fpga_slot0].len << spi_flash_log2_sector_size();
    return true;

  case QLD_DOM_FW:
    if (slot_num >= fd->fw_nslots){
      *status = QLDS_INVALID_ARG;
      return false;
    }
    *slot_start = fd->slot[slot_num + fd->fw_slot0].start << spi_flash_log2_sector_size();
    *slot_len = fd->slot[slot_num + fd->fw_slot0].len << spi_flash_log2_sector_size();
    return true;

  default:
    *status = QLDS_INVALID_ARG;
    return false;
  }
}


static bool
check_flashdir(void)
{
  return get_flashdir() != 0;
}

void
loader_parser(const unsigned char *input, size_t ilen,
	      unsigned char *output, size_t max_olen, size_t *actual_olen)
{
  //assert (max_olen >= 8);
  if (!(max_olen >= 8))
    abort();

  *actual_olen = 0;
  uint32_t status = QLDS_BAD_PKT;

  uint32_t cmd = get32(input);
  uint32_t nonce = get32(input+4);
  uint32_t slot = 0;
  uint32_t addr = 0;
  uint32_t len = 0;

  if (ilen < 8){
    nonce = -1;
    goto done;
  }

  uint32_t slot_start;		// offset in flash
  uint32_t slot_len;		// length in bytes
  
  if (ilen >= 5 * sizeof(uint32_t)){
    slot = get32(input+8);
    addr = get32(input+12);
    len = get32(input+16);
  }

  switch (cmd){
  case QLD_FLASH_ERASE_START:
    // <QLD_FLASH_ERASE_START> <nonce> <slot> <addr> <len>
    if (ilen != 5 * sizeof(uint32_t))
      goto done;

    if (!check_flashdir()){
      status = QLDS_BAD_FLASHDIR;
      goto done;
    }
    if (!map_slot(slot, &slot_start, &slot_len, &status))
      goto done;

    if (QLD_SLOT_DOM(slot) != QLD_DOM_UNMAPPED){
      addr = slot_start;
      len = slot_len;
    }
    //printf("flash_erase: addr = 0x%x, len=0x%x\n", addr, len);

    if (0 && erased_p(addr, len)){	// already erased?
      async_state.first = async_state.last = async_state.current = 0;
      goto ok;
    }

    if (erase_flash(addr, len))
      goto ok;

    status = QLDS_FAILED;
    goto done;
    

  case QLD_FLASH_ERASE_POLL:
    // <QLD_FLASH_ERASE_POLL> <nonce>
    if (ilen != 2 * sizeof(uint32_t))
      goto done;

    if (spi_flash_async_erase_poll(&async_state))
      goto ok;

    status = QLDS_BUSY;
    goto done;


  case QLD_FLASH_WRITE:
    // <QLD_FLASH_WRITE> <nonce> <slot> <addr> <len> <data ...>
    if (ilen < 5 * sizeof(uint32_t))
      goto done;

    if (ilen != 5 * sizeof(uint32_t) + len)
      goto done;

    if (!check_flashdir()){
      status = QLDS_BAD_FLASHDIR;
      goto done;
    }
    if (!map_slot(slot, &slot_start, &slot_len, &status))
      goto done;

    addr += slot_start;
    len = min(len, slot_len);

    if (spi_flash_program(addr, len, &input[5*sizeof(uint32_t)]))
      goto ok;

    status = QLDS_FAILED;
    goto done;
    

  case QLD_FLASH_READ:
  case QLD_MEM_READ:
  case QLD_MEM_WRITE:
  case QLD_GOTO:
    status = QLDS_NOTIMPLEMENTED;
    goto done;

  case QLD_PING:
    // <QLD_PING> <nonce>
    if (ilen != 2 * sizeof(uint32_t))
      goto done;
    goto ok;

#if 0
  case QLD_EEPROM_SET_XXX:
  // <QLD_EEPROM_SET_XXX> <nonce> <arg> <idlen> <idstr> <data ...>
  {
    uint32_t arg    = get32(input+2*sizeof(uint32_t));
    uint32_t idlen  = get32(input+3*sizeof(uint32_t));
    uint8_t *idstr  = (uint8_t*)input+4*sizeof(uint32_t);
    uint8_t *data_p = idstr+idlen;
    
    //handle the ethernet cases
    if (strncmp((char*)idstr, "ip", idlen) == 0){
        struct ip_addr addr = {get32(data_p)};
        ethernet_set_ip_addr(arg, addr);
    }
    else if (strncmp((char*)idstr, "mac", idlen) == 0){
        eth_mac_addr_t addr;
        memcpy(&addr, data_p, sizeof(addr));
        ethernet_set_mac_addr(arg, &addr);
    }
    //handle the main board eeprom
    else if (strncmp((char*)idstr, "qrrev", idlen) == 0){
        qr_set_revision(get32(data_p));
    }
    else if (strncmp((char*)idstr, "qrser", idlen) == 0){
        qr_set_serial(get32(data_p));
    }
    else if (strncmp((char*)idstr, "qrmod", idlen) == 0){
        qr_set_model(get32(data_p));
    }
    //handle the caldiv eeprom
    else if (strncmp((char*)idstr, "cdrev", idlen) == 0){
        if (_caldiv_set_rev) _caldiv_set_rev(get32(data_p));
    }
    else if (strncmp((char*)idstr, "cdser", idlen) == 0){
        if (_caldiv_set_ser) _caldiv_set_ser(get32(data_p));
    }
    else if (strncmp((char*)idstr, "cdmod", idlen) == 0){
        if (_caldiv_set_ser) _caldiv_set_mod(get32(data_p));
    }
    else {
        goto done;
    }
  }
  goto ok;
#endif

  default:
    status = QLDS_UNKNOWN_CMD;
    goto done;
  }

 ok:
  status = QLDS_OK;

 done:
  put32(output, nonce);
  put32(output+4, status);
  *actual_olen = 2*sizeof(uint32_t);
}
