//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("%s flags\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 0x1\n", argv[0]);

}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep;
	u32 flags;

	if (argc != 2) {
		usage(argv);
		return EXIT_FAILURE;
	}

	errno = 0;
	flags = strtol(argv[1], NULL, 16);
	if (errno) {
		printf("Failed to convert flags, make sure you enter as hex\n");
		return EXIT_FAILURE;
	}

	ep = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	if (!ep) {
		printf("eeprom not found\n");
		return EXIT_FAILURE;
	}

	usrp_sulfur_eeprom_print(ep);

	/* Set flag */
	ep->mcu_flags[0] = htonl(flags);
	usrp_sulfur_eeprom_recrc(ep);

	/* Write out to eeprom */
	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");
	free(ep);

	printf("-- Reading back \n");
	ep = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	usrp_sulfur_eeprom_print(ep);
	free(ep);
}
