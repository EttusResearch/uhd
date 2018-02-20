//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
	struct usrp_sulfur_db_eeprom *ep;
	int which_slot = 0;

	if (argc != 2)
		return EXIT_FAILURE;

	which_slot = atoi(argv[1]);

	if (!which_slot)
		ep = usrp_sulfur_db_eeprom_from_file(NVMEM_PATH_SLOT_A);
	else
		ep = usrp_sulfur_db_eeprom_from_file(NVMEM_PATH_SLOT_B);

	if (!ep)
		return EXIT_FAILURE;
	usrp_sulfur_db_eeprom_print(ep);
	free(ep);

	return 0;
}
