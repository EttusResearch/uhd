//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("Example:\n");
	printf("%s -c -f\n", argv[0]);
}


int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep;
	int erase = 0;

	if (argc == 3) {
		erase = !strcmp("-c", argv[1]) &&
			!strcmp("-f", argv[2]);
	} else {
		usage(argv);
		return EXIT_FAILURE;
	}

	if (!erase)
		return EXIT_FAILURE;

	printf("Erasing ...");
	ep = malloc(sizeof(*ep));
	memset(ep, 0xff, sizeof(*ep));

	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");
	printf(" Done\n");
	free(ep);

	return EXIT_SUCCESS;
}
