// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * All eeproms on this platform are 256 bytes long
 */
#define TLV_EEPROM_SIZE (256)

/**
 * struct tlv_eeprom - structure describing the eeprom layout
 *
 * NOTE: All fields stored to eeprom are little endian, and this library
 * assumes the processor running it is also little endian (i.e., no endianess
 * conversion is performed).
 *
 * @magic: magic to identify the eeprom
 * @crc: crc over the size field plus the following size bytes
 * @size: size of remaining data in bytes
 * @tlv: up to 244 bytes of tlv data
 */
struct tlv_eeprom {
	uint32_t magic;
	uint32_t crc;
	uint32_t size;
	uint8_t  tlv[TLV_EEPROM_SIZE - 3 * sizeof(uint32_t)];
} __attribute__((packed));

/**
 * tlv_eeprom_init - create tlv_eeprom from backing store
 * @data: backing store, which must be at least TLV_EEPROM_SIZE bytes
 */
struct tlv_eeprom *tlv_eeprom_init(void *data);

/**
 * tlv_eeprom_validate - check if an eeprom is valid
 * @e: eeprom to validate
 * @magic: expected magic for this eeprom
 *
 * Checks to see whether the eeprom is valid. This validates the magic, the
 * size, and the CRC of the data.
 *
 * Return: zero on success, -1 for a magic mismatch, -2 for an invalid size, -3
 *         for an invalid crc
 */
int tlv_eeprom_validate(const struct tlv_eeprom *e, uint32_t magic);

/**
 * tlv_eeprom_seal - seal an eeprom
 * @e: eeprom to seal
 * @magic: magic to use for this eeprom
 * @len: size of tlv data stored in eeprom
 *
 * Updates the magic, size, and crc of the eeprom. This should be called prior
 * to writing an eeprom.
 */
void tlv_eeprom_seal(struct tlv_eeprom *e, uint32_t magic, size_t len);

/*
 * tlv_eeprom_crc - return the CRC for the eeprom
 * @eeprom: eeprom to compute the crc
 *
 * Note: e->size must be valid!
 *
 * Returns: the crc
 */
extern uint32_t tlv_eeprom_crc(const struct tlv_eeprom *e);

/**
 * tlv_write - writes a tlv tuple to the buffer
 * @buf: buffer to write to
 * @tag: tag for this data
 * @len: length of value
 * @val: value to write
 *
 * Return: number of bytes written to buffer, including tag and size
 */
size_t tlv_write(void *buf, uint8_t tag, uint8_t len, const void *val);

/**
 * tlv_lookup - lookup the value associated with a tag
 * @buf: buffer containing tlv data
 * @bufsz: size of buffer containing tlv data
 * @tag: tag to lookup
 *
 * Return: pointer to value at tag, or NULL if tag was not found
 */
const void *tlv_lookup(const void *buf, size_t bufsz, uint8_t tag);

/*
 * tlv_for_each - call fn for each value in buffer
 * @buf: buffer containing tlv data
 * @bufsz: size of buffer containing tlv data
 * @fn: function to invoke for each value
 */
void tlv_for_each(const void *buf, size_t bufsz,
		  void (*fn)(uint8_t tag, uint8_t len, const void *val));

