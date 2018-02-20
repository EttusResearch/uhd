//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdlib.h>
#include <stdio.h>

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("%s slot pid rev serial#\n\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 0 0x0150 0 310A850\n",
		argv[0]);

}


int main(int argc, char *argv[])
{
	struct usrp_sulfur_db_eeprom *ep;
	int which_slot = 0;

	if (argc != 5) {
		usage(argv);
		return EXIT_FAILURE;
	}

	which_slot = atoi(argv[1]);

	ep = usrp_sulfur_db_eeprom_new(strtol(argv[2], NULL, 16), atoi(argv[3]), argv[4]);
	usrp_sulfur_db_eeprom_print(ep);

	if (!which_slot)
		usrp_sulfur_db_eeprom_to_file(ep, NVMEM_PATH_SLOT_A);
	else
		usrp_sulfur_db_eeprom_to_file(ep, NVMEM_PATH_SLOT_B);

	return 0;
}

