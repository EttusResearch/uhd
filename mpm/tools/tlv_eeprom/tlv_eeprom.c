// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#include <string.h>
#include "tlv_eeprom.h"

struct tlv_eeprom *tlv_eeprom_init(void *data)
{
	struct tlv_eeprom *e = data;

	return e;
}

void tlv_eeprom_seal(struct tlv_eeprom *e, uint32_t magic, size_t len)
{
	e->magic = magic;
	e->size = len;
	e->crc = tlv_eeprom_crc(e);
}

int tlv_eeprom_validate(const struct tlv_eeprom *e, uint32_t magic)
{
	uint32_t crc;

	if (e->magic != magic)
		return -1;

	if (e->size > TLV_EEPROM_SIZE)
		return -2;

	crc = tlv_eeprom_crc(e);
	if (e->crc != crc)
		return -3;

	return 0;
}

struct tlv {
	uint8_t tag;
	uint8_t len;
	uint8_t val[0];
};


static inline const struct tlv *tlv_next(const struct tlv *tlv, const void *end)
{
	const void *p = &tlv->val;

	p += tlv->len;
	if (p >= end)
		return NULL;
	return p;
}

static inline const struct tlv *tlv_find(const struct tlv *tlv,
					 const void *end, uint8_t tag)
{
	do {
		if (tlv->tag == tag)
			break;
	} while ((tlv = tlv_next(tlv, end)));

	return tlv;
}

void tlv_for_each(const void *buf, size_t bufsz,
		  void (*fn)(uint8_t tag, uint8_t len, const void *val))
{
	const struct tlv *tlv = buf;

	do {
		fn(tlv->tag, tlv->len, tlv->val);
	} while ((tlv = tlv_next(tlv, buf + bufsz)));
}

const void *tlv_lookup(const void *buf, size_t bufsz, uint8_t tag)
{
	const struct tlv *tlv;

	tlv = tlv_find(buf, buf + bufsz, tag);
	if (!tlv)
		return tlv;

	return tlv->val;
}

size_t tlv_write(void *buf, uint8_t tag, uint8_t len, const void *val)
{
	struct tlv *tlv = buf;

	tlv->tag = tag;
	tlv->len = len;
	memcpy(tlv->val, val, len);

	return sizeof(*tlv) + len;
}
