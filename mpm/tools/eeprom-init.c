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
	printf("%s serial# revision eth0 eth1 eth2 pid\n\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 310A850 2 0c:22:cc:1a:25:c1 0c:22:cc:1a:25:c2 0c:22:cc:1a:25:c3 0x4242\n",
		argv[0]);

}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep, *ep2;

	if (argc < 6 || argc > 7) {
		usage(argv);
		return EXIT_FAILURE;
	}

	printf("sizeof(*ep)=%lu\n", sizeof(*ep));

	long pid = 0x4242;
	if (argc == 7) {
		pid = strtol(argv[6], NULL, 0);
	}

	if (pid < 0 || pid > 0xFFFF) {
		printf("Invalid PID: %lX\n", pid);
		return EXIT_FAILURE;
	}

	ep = usrp_sulfur_eeprom_new(NULL, (u16) pid, atoi(argv[2]), (const u8*) argv[1],
			            argv[3], argv[4], argv[5]);
	usrp_sulfur_eeprom_print(ep);
	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");
	free(ep);

	printf("-- Reading back \n");
	ep2 = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	usrp_sulfur_eeprom_print(ep2);
	free(ep2);
}
