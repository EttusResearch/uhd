//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef EEPROM_H
#define EEPROM_H

#include <arpa/inet.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define ETH_ALEN 6

/* TODO: Come up with a smarter way to do this when we start using this on
 * another device */
#define NVMEM_PATH_SLOT_A "/sys/bus/nvmem/devices/9-00500/nvmem"
#define NVMEM_PATH_SLOT_B "/sys/bus/nvmem/devices/10-00500/nvmem"
#define NVMEM_PATH_MB "/sys/bus/nvmem/devices/2-00500/nvmem"

struct usrp_sulfur_eeprom {
	u32 magic;
	u32 version;
	u32 mcu_flags[4];
	u16 pid;
	u16 rev;
	u8 serial[8];
	u8 eth_addr0[ETH_ALEN];
	u16 dt_compat;
	u8 eth_addr1[ETH_ALEN];
	u16 mcu_compat;
	u8 eth_addr2[ETH_ALEN];
	u8 __pad_2[2];
	u32 crc;
} __attribute__((packed));

struct db_rev {
	u8 rev;
	u8 dt_compat;
} __attribute__((packed));

struct usrp_sulfur_db_eeprom {
	u32 magic;
	u32 version;
	u16 pid;
	union rev {
		u16 v1_rev;
		struct db_rev v2_rev;
	} rev;
	char serial[8];
	u32 crc;
} __attribute__((packed));

/* Motherboard EEPROM stuff */
struct usrp_sulfur_eeprom *usrp_sulfur_eeprom_new(const u32 *mcu_flags,
						  const u16 pid,
						  const u16 rev,
						  const char *serial,
						  const char *eth_addr0,
						  const char *eth_addr1,
						  const char *eth_addr2,
						  const u16 dt_compat,
						  const u16 mcu_compat);

void usrp_sulfur_eeprom_to_i2c(struct usrp_sulfur_eeprom *ep, const char *path);

void usrp_sulfur_eeprom_to_file(struct usrp_sulfur_eeprom *ep,
				const char *path);

void usrp_sulfur_eeprom_recrc(struct usrp_sulfur_eeprom *ep);

struct usrp_sulfur_eeprom *usrp_sulfur_eeprom_from_file(const char *path);

void usrp_sulfur_eeprom_print(const struct usrp_sulfur_eeprom *ep);

/* Daughterboard EEPROM stuff */
struct usrp_sulfur_db_eeprom *usrp_sulfur_db_eeprom_new(const u16 pid,
							const u16 rev,
							const char *serial,
							const u16 dt_compat);

void usrp_sulfur_db_eeprom_to_file(struct usrp_sulfur_db_eeprom *ep,
				   const char *path);

struct usrp_sulfur_db_eeprom *usrp_sulfur_db_eeprom_from_file(const char *path);

void usrp_sulfur_db_eeprom_print(const struct usrp_sulfur_db_eeprom *ep);

#endif /* EEPROM_H */
