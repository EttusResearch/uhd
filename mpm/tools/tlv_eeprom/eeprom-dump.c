// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#include <stdio.h>
#include <stdlib.h>

#include "tlv_eeprom.h"
#include "tlv_eeprom_io.h"
#include "usrp_eeprom.h"

int main(int argc, char **argv)
{
	struct tlv_eeprom *eeprom;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <eeprom>\n", argv[0]);
		return 1;
	}

	eeprom = tlv_eeprom_read_from_file(argv[1]);
	if (!eeprom)
		return 1;

	if (!tlv_eeprom_validate(eeprom, USRP_EEPROM_MAGIC))
		tlv_for_each(eeprom->tlv, eeprom->size, usrp_eeprom_trace);
	else
		fprintf(stderr, "eeprom contents invalid!\n");

	free(eeprom);

	return 0;
}
