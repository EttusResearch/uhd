//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdio.h>
#include <stdlib.h>

int derive_dt_compat(int rev)
{
	/* up to rev6 they were individual dts */
	if (rev > 5)
		return 5;

	return rev;
}

int derive_mcu_compat(int rev)
{
	/* up to rev6 they were individual firmware */
	if (rev > 5)
		return 5;

	return rev;
}

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("%s serial# revision eth0 eth1 eth2 [pid] [dt-compat] [mcu-compat]\n\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 310A850 2 0c:22:cc:1a:25:c1 0c:22:cc:1a:25:c2 0c:22:cc:1a:25:c3 0x4242\n",
		argv[0]);
	printf("or specifying dt-compat and mcu-compat explicitly:\n");
	printf("$ %s 310A850 2 0c:22:cc:1a:25:c1 0c:22:cc:1a:25:c2 0c:22:cc:1a:25:c3 0x4242 5 5\n",
		argv[0]);
}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep, *ep2;
	u16 dt_compat = 0;
	u16 mcu_compat = 0;

	if (argc < 6 || argc > 9) {
		usage(argv);
		return EXIT_FAILURE;
	}

	long pid = 0x4242;
	if (argc >= 7) {
		pid = strtol(argv[6], NULL, 0);
	}

	if (argc >= 8) {
		dt_compat = strtol(argv[7], NULL, 0);
		printf("dt_compat=%u\n", dt_compat);
	}

	if (argc == 9) {
		mcu_compat = strtol(argv[8], NULL, 0);
		printf("mcu_compat=%u\n", mcu_compat);
	}

	if (pid < 0 || pid > 0xFFFF) {
		printf("Invalid PID: %lX\n", pid);
		return EXIT_FAILURE;
	}

	/* If no MCU or DT compat specified, derive based on rule up there,
	 * i.e. everything newer than 5 will be 5, assuming we don't change
	 * anything software visible anymore
	 */
	ep = usrp_sulfur_eeprom_new(NULL, (u16) pid, atoi(argv[2]), argv[1],
			            argv[3], argv[4], argv[5], dt_compat ? dt_compat : derive_dt_compat(atoi(argv[2])),
				    mcu_compat ? mcu_compat : derive_mcu_compat(atoi(argv[2])));

	usrp_sulfur_eeprom_print(ep);
	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");
	free(ep);

	printf("-- Reading back \n");
	ep2 = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	usrp_sulfur_eeprom_print(ep2);
	free(ep2);
}
