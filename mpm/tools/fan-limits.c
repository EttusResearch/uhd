//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdio.h>
#include <stdlib.h>

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("%s fan0_min fan0_max fan1_min fan1_max\n\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 3800 13000 3800 1300\n", argv[0]);
}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep;
	uint8_t fan0_max, fan0_min, fan1_max, fan1_min;
	uint32_t new_flags = 0;

	if (argc != 5) {
		usage(argv);
		return EXIT_FAILURE;
	}

	fan0_min = atoi(argv[1]) / 100;
	fan0_max = atoi(argv[2]) / 100;
	fan1_min = atoi(argv[3]) / 100;
	fan1_max = atoi(argv[4]) / 100;

	printf("Writing: %d/%d/%d/%d\n", fan0_min, fan0_max, fan1_min, fan1_max);

	ep = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	ep->mcu_flags[0] = 0;
	ep->mcu_flags[1] = htonl(fan1_max << 24 | fan1_min << 16 | fan0_max << 8 | fan0_min);
	usrp_sulfur_eeprom_recrc(ep);
	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");


	free(ep);

	return 0;
}
