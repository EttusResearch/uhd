/*
 * Copyright 2010-2012 Ettus Research LLC
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
#include <string.h>
#include "i2c.h"
#include "usrp2/fw_common.h"

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

typedef struct{
    eth_mac_addr_t mac_addr;
    struct ip_addr ip_addr;
    struct ip_addr gateway;
    struct ip_addr subnet;
} eth_addrs_t;

static bool eth_addrs_initialized = false;

static const eth_addrs_t default_eth_addrs = {
    .mac_addr = {{0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff}},
    .ip_addr = {(192 << 24 | 168 << 16 | 10  << 8  | 2 << 0)},
    .gateway = {(192 << 24 | 168 << 16 | 10  << 8  | 1 << 0)},
    .subnet  = {(255 << 24 | 255 << 16 | 255 << 8  | 0 << 0)},
};

static eth_addrs_t current_eth_addrs;

static void eth_addrs_init(void){
    if (eth_addrs_initialized) return;
    eth_addrs_initialized = true;

    #define eth_addrs_init_x(addr, x){ \
        const bool ok = eeprom_read(USRP2_I2C_ADDR_MBOARD, addr, &current_eth_addrs.x, sizeof(current_eth_addrs.x)); \
        if (!ok || unprogrammed(&current_eth_addrs.x, sizeof(current_eth_addrs.x))){ \
            memcpy(&current_eth_addrs.x, &default_eth_addrs.x, sizeof(current_eth_addrs.x)); \
        } \
    }

    eth_addrs_init_x(USRP2_EE_MBOARD_MAC_ADDR, mac_addr);
    eth_addrs_init_x(USRP2_EE_MBOARD_IP_ADDR,  ip_addr);
    eth_addrs_init_x(USRP2_EE_MBOARD_GATEWAY,  gateway);
    eth_addrs_init_x(USRP2_EE_MBOARD_SUBNET,   subnet);

}

const eth_mac_addr_t *ethernet_mac_addr(void){
    eth_addrs_init();
    return &current_eth_addrs.mac_addr;
}

const struct ip_addr *get_ip_addr(void){
    eth_addrs_init();
    return &current_eth_addrs.ip_addr;
}

const struct ip_addr *get_subnet(void){
    eth_addrs_init();
    return &current_eth_addrs.subnet;
}

const struct ip_addr *get_gateway(void){
    eth_addrs_init();
    return &current_eth_addrs.gateway;
}

bool set_ip_addr(const struct ip_addr *t){
    const bool ok = eeprom_write(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_IP_ADDR, t, sizeof(struct ip_addr));
    if (ok) current_eth_addrs.ip_addr = *t;
    return ok;
}

void eth_addrs_set_default(void){
    eth_addrs_initialized = true;
    memcpy(&current_eth_addrs, &default_eth_addrs, sizeof(default_eth_addrs));
}
