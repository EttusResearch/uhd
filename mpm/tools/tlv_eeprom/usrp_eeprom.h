// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#pragma once

#include <stdint.h>

#define USRP_EEPROM_MAGIC 0x55535250

/**
 * struct usrp_eeprom_board_info - common board info
 *
 * @pid: product id for the board
 * @rev: current hardware revision, starting at 1
 * @compat_rev: last hardware revision this is compatible with
 * @serial: NUL-terminated serial number string
 */
#define USRP_EEPROM_BOARD_INFO_TAG (0x10)
struct usrp_eeprom_board_info {
	uint16_t pid;
	uint16_t rev;
	uint16_t compat_rev;
	char serial[8];
} __attribute__((packed));

/**
 * struct usrp_eeprom_module_info - common module info
 *
 * @pid: product id for the module
 * @rev: module revision, starting at 1
 * @serial: NUL-terminated serial number string for the module
 */
#define USRP_EEPROM_MODULE_INFO_TAG (0x11)
struct usrp_eeprom_module_info {
	uint16_t pid;
	uint16_t rev;
	char serial[8];
} __attribute__((packed));

/**
 * struct usrp_eeprom_mac_addr - mac address
 *
 * @addr: 6-byte MAC address
 */

/* tags 0xA0 through 0xAF are reserved for MAC addresses */
#define USRP_EEPROM_MAX_MAC_ADDRS 0xF
#define USRP_EEPROM_MAC_ADDR_TAG(x) (0xA0 + ((x) & USRP_EEPROM_MAX_MAC_ADDRS))
#define USRP_EEPROM_ETH0_ADDR_TAG  USRP_EEPROM_MAC_ADDR_TAG(0)
#define USRP_EEPROM_QSFP0_ADDR_TAG USRP_EEPROM_MAC_ADDR_TAG(1)
#define USRP_EEPROM_QSFP1_ADDR_TAG USRP_EEPROM_MAC_ADDR_TAG(2)
struct usrp_eeprom_mac_addr {
	uint8_t addr[6];
} __attribute__((packed));

/**
 * struct usrp_eeprom_db_pwr_seq - daughterboard power sequence
 *
 * @nsteps: the number of steps in the sequence
 * @steps.delay: delay in milliseconds to wait after enabling supplies
 * @steps.supply_mask: bitmask of supplies to enable
 *	For X410: bit 0 = 1.8, bit 1 = 2.5, 3.3, 3.7, 12
 */
#define USRP_EEPROM_DB_PWR_SEQ_TAG (0x12)
struct usrp_eeprom_db_pwr_seq {
	uint8_t nsteps;
	struct {
		uint16_t delay;
		uint8_t supply_mask;
	} steps[8];
};

#define USRP_EEPROM_MCU_FLAGS (0x20)
struct usrp_eeprom_mcu_flags {
	uint8_t flags[6];
} __attribute__((packed));

/**
 * struct usrp_eeprom_fan_limits - fan speed (rpm) limits
 *
 * @min: minimum configurable speed
 * @start: necessary start speed
 * @max: maximum configurable speed
 */
#define USRP_EEPROM_FAN_LIMITS (0x21)
struct usrp_eeprom_fan_limits {
	uint16_t min;
	uint16_t start;
	uint16_t max;
} __attribute__((packed));

/**
 * struct usrp_eeprom_fan_fixed_capacity - fixed cooling capacity
 *
 * @capacity: fixed cooling capacity percentage. Range 0-100.
 * @reserved: extra byte for alignment
 */
#define USRP_EEPROM_FAN_FIXED_CAPACITY (0x22)
struct usrp_eeprom_fan_fixed_capacity {
	uint8_t capacity;
	uint8_t reserved; /* for natural alignment */
} __attribute__((packed));

/**
 * struct usrp_eeprom_clkaux_tuning_word - clk aux dac tuning word
 *
 * @tuning_word: clocking aux board dac tuning word. Range 0-1023.
 */
#define USRP_EEPROM_CLKAUX_TUNING_WORD (0x23)
struct usrp_eeprom_clkaux_tuning_word {
	uint16_t tuning_word;
} __attribute__((packed));

#include <stdio.h>
#include <assert.h>

static void usrp_eeprom_trace(uint8_t tag, uint8_t len, const void *val)
{
	uint8_t i;

	switch (tag) {
	case USRP_EEPROM_BOARD_INFO_TAG:
	{
		const struct usrp_eeprom_board_info *v = val;
		assert(sizeof(*v) == len);
		printf("%s (0x%02x) ", "usrp_eeprom_board_info", tag);
		printf("pid: 0x%04x, rev: 0x%04x, compat_rev: 0x%04x, serial: %s\n",
		       v->pid, v->rev, v->compat_rev, v->serial);
	}
	break;
	case USRP_EEPROM_MODULE_INFO_TAG:
	{
		const struct usrp_eeprom_module_info *v = val;
		assert(sizeof(*v) == len);
		printf("%s (0x%02x) ", "usrp_eeprom_module_info", tag);
		printf("pid: 0x%04x, rev: 0x%04x, serial: %s\n",
		       v->pid, v->rev, v->serial);
	}
	break;
	case USRP_EEPROM_MAC_ADDR_TAG(0) ... USRP_EEPROM_MAC_ADDR_TAG(USRP_EEPROM_MAX_MAC_ADDRS):
	{
		const struct usrp_eeprom_mac_addr *v = val;
		assert(sizeof(*v) == len);
		printf("%s mac_addr_%d (0x%02x) ", "usrp_eeprom_mac_addr",
		       tag - USRP_EEPROM_MAC_ADDR_TAG(0), tag);
		for (i = 0; i < 6; i++)
			printf("%02x%c", v->addr[i], i == 5 ? ' ' : ':');
		printf("\n");
	}
	break;
	case USRP_EEPROM_DB_PWR_SEQ_TAG:
	{
		const struct usrp_eeprom_db_pwr_seq *v = val;
		assert(sizeof(*v) == len);
		printf("%s (0x%02x) ", "usrp_eeprom_db_pwr_seq", tag);
		for (i = 0; i < 8; i++)
			printf("(%u, 0x%02x) ", v->steps[i].delay, v->steps[i].supply_mask);
		printf("\n");
	}
	break;
	case USRP_EEPROM_MCU_FLAGS:
	{
		const struct usrp_eeprom_mcu_flags *v = val;
		printf("%s (0x%02x) ", "usrp_eeprom_mcu_flags", tag);
		for (i = 0; i < 6; i++)
			printf("0x%02x ", v->flags[i]);
		printf("\n");
	}
	break;
	case USRP_EEPROM_FAN_LIMITS:
	{
		const struct usrp_eeprom_fan_limits *v = val;
		printf("%s (0x%02x) ", "usrp_eeprom_fan_limits", tag);
		printf("min: %d, start: %d, max: %d", v->min, v->start, v->max);
		printf("\n");
	}
	break;
	case USRP_EEPROM_FAN_FIXED_CAPACITY:
	{
		const struct usrp_eeprom_fan_fixed_capacity *v = val;
		printf("%s (0x%02x) ", "usrp_eeprom_fan_fixed_capacity", tag);
		printf("%d", v->capacity);
		printf("\n");
	}
	break;
	case USRP_EEPROM_CLKAUX_TUNING_WORD:
	{
		const struct usrp_eeprom_clkaux_tuning_word *v = val;
		printf("%s (0x%02x) ", "usrp_eeprom_clkaux_tuning_word", tag);
		printf("%d", v->tuning_word);
		printf("\n");
	}
	break;
	default:
	{
		const uint8_t *ptr = val;
		printf("%s (0x%02x) len: %hhu, val: ", "unknown", tag, len);
		for (i = 0; i < len; i++)
			printf("%02x ", ptr[i]);
		printf("\n");
		break;
	}
	}
}
