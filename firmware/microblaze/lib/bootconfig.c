/* -*- c -*- */
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "bootconfig.h"
#include "bootconfig_private.h"
#include <stdint.h>
#include <stddef.h>
#include <i2c.h>
#include <quadradio/i2c_addr.h>
#include <mdelay.h>
#include <xilinx_v5_icap.h>
#include <nonstdio.h>

eeprom_boot_info_t eeprom_shadow;

static eeprom_boot_info_t eeprom_default = {
  .magic = EEPROM_BOOT_INFO_MAGIC,
  .nattempts = 1,
  .next_boot.fpga_image_number = 0,
  .next_boot.firmware_image_number = 0,
  .default_boot.fpga_image_number = 0,
  .default_boot.firmware_image_number = 0
};

eeprom_boot_info_t *
_bc_get_eeprom_shadow(void)
{
  return &eeprom_shadow;
}


bool
_bc_write_eeprom_shadow(void)
{
  return eeprom_write(I2C_ADDR_MBOARD, BOOT_INFO_OFFSET, &eeprom_shadow, sizeof(eeprom_shadow));
}

void
bootconfig_init(void)
{
  if (!eeprom_read(I2C_ADDR_MBOARD, BOOT_INFO_OFFSET, &eeprom_shadow, sizeof(eeprom_shadow))
      || eeprom_shadow.magic != EEPROM_BOOT_INFO_MAGIC){
    eeprom_shadow = eeprom_default;
    _bc_write_eeprom_shadow();
  }
}

bootconfig_t 
bootconfig_get_default(void)
{
  return eeprom_shadow.default_boot;
}

bool
bootconfig_set_default(bootconfig_t bc)
{
  if (!validate_bootconfig(bc))
    return false;

  eeprom_shadow.default_boot  = bc;
  eeprom_shadow.next_boot  = bc;
  return _bc_write_eeprom_shadow();
}

void
bootconfig_boot(bootconfig_t bc)
{
  if (!validate_bootconfig(bc))
    return;

  eeprom_shadow.next_boot = bc;
  eeprom_shadow.nattempts = 1;
  _bc_write_eeprom_shadow();

  if (1){
    puts("\nbootconfig: chaining to FPGA slot 0 bootloader");
    mdelay(100);
  }

  while (1){
    // Reload fpga with code from SPI flash address 0x0.
    icap_reload_fpga(0x00000000);
  }
}  
