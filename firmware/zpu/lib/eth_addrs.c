/*
 * Copyright 2010 Ettus Research LLC
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

#include "ethernet.h"
#include "memory_map.h"
#include "nonstdio.h"
#include <stdbool.h>
#include "i2c.h"
#include "usrp2/fw_common.h"

////////////////////////////////////////////////////////////////////////
// EEPROM Layout
////////////////////////////////////////////////////////////////////////
#define USRP2_EE_MBOARD_MAC_ADDR 0x02 //6 bytes
#define USRP2_EE_MBOARD_IP_ADDR  0x0C //uint32, big-endian

static bool
unprogrammed(const void *t, size_t len)
{
  int i;
  uint8_t *p = (uint8_t *)t;
  bool all_zeros = true;
  bool all_ones =  true;
  for (i = 0; i < len; i++){
    all_zeros &= p[i] == 0x00;
    all_ones  &= p[i] == 0xff;
  }
  return all_ones | all_zeros;
}

//////////////////// MAC Addr Stuff ///////////////////////

static bool src_mac_addr_initialized = false;

static const eth_mac_addr_t default_mac_addr = {{
    0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff
  }};

static eth_mac_addr_t src_mac_addr = {{
    0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff
  }};
  
void set_default_mac_addr(void)
{
    src_mac_addr_initialized = true;
    src_mac_addr = default_mac_addr;
}

const eth_mac_addr_t *
ethernet_mac_addr(void)
{
  if (!src_mac_addr_initialized){    // fetch from eeprom
    src_mac_addr_initialized = true;

    // if we're simulating, don't read the EEPROM model, it's REALLY slow
    if (hwconfig_simulation_p())
      return &src_mac_addr;

    eth_mac_addr_t tmp;
    bool ok = eeprom_read(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_MAC_ADDR, &tmp, sizeof(tmp));
    if (!ok || unprogrammed(&tmp, sizeof(tmp))){
      // use the default
    }
    else
      src_mac_addr = tmp;
  }

  return &src_mac_addr;
}

bool
ethernet_set_mac_addr(const eth_mac_addr_t *t)
{
  bool ok = eeprom_write(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_MAC_ADDR, t, sizeof(eth_mac_addr_t));
  if (ok){
    src_mac_addr = *t;
    src_mac_addr_initialized = true;
    //eth_mac_set_addr(t); //this breaks the link
  }

  return ok;
}

//////////////////// IP Addr Stuff ///////////////////////

static bool src_ip_addr_initialized = false;

static const struct ip_addr default_ip_addr = {
    (192 << 24 | 168 << 16 | 10 << 8 | 2 << 0)
};

static struct ip_addr src_ip_addr = {
    (192 << 24 | 168 << 16 | 10 << 8 | 2 << 0)
};

void set_default_ip_addr(void)
{
    src_ip_addr_initialized = true;
    src_ip_addr = default_ip_addr;
}

const struct ip_addr *get_ip_addr(void)
{
  if (!src_ip_addr_initialized){    // fetch from eeprom
    src_ip_addr_initialized = true;

    // if we're simulating, don't read the EEPROM model, it's REALLY slow
    if (hwconfig_simulation_p())
      return &src_ip_addr;

    struct ip_addr tmp;
    bool ok = eeprom_read(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_IP_ADDR, &tmp, sizeof(tmp));
    if (!ok || unprogrammed(&tmp, sizeof(tmp))){
      // use the default
    }
    else
      src_ip_addr = tmp;
  }

  return &src_ip_addr;
}

bool set_ip_addr(const struct ip_addr *t){
  bool ok = eeprom_write(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_IP_ADDR, t, sizeof(struct ip_addr));
  if (ok){
    src_ip_addr = *t;
    src_ip_addr_initialized = true;
  }

  return ok;
}
