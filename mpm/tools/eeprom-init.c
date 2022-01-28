//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include "eeprom-pids.h"
#include <stdio.h>
#include <stdlib.h>

int derive_rev_compat(int rev, long pid)
{
	if (pid == E320_PID) {
		if (rev >= 2)
			return 2;
	} else { /* N3x0 */
		if (rev > 5)
			return 5;
	}
	return rev;
}

int derive_dt_compat(int rev, long pid)
{
	if (pid == E320_PID) {
		return 0;
	} else { /* N3x0 */
		/* up to rev5 they were individual dts */
		if (rev > 5)
			return 5;
	}
	return rev;
}

int derive_mcu_compat(int rev, long pid)
{
	if (pid == E320_PID) {
		return 2;
	} else { /* N3x0 */
		/* up to rev5 they were individual firmware */
		if (rev > 5)
			return 5;
	}

	return rev;
}

void usage(char *argv[])
{
	printf("-- Usage -- \n");
	printf("%s serial# revision eth0 eth1 eth2 [pid] [dt-compat] [mcu-compat] [rev-compat]\n\n", argv[0]);
	printf("Example:\n");
	printf("$ %s 310A850 2 0c:22:cc:1a:25:c1 0c:22:cc:1a:25:c2 0c:22:cc:1a:25:c3 0x4242\n",
		argv[0]);
	printf("or specifying dt-compat, mcu-compat, and rev-compat explicitly:\n");
	printf("$ %s 310A850 2 0c:22:cc:1a:25:c1 0c:22:cc:1a:25:c2 0c:22:cc:1a:25:c3 0x4242 5 5 5\n",
		argv[0]);
	printf("\n");
	printf("Note: 'pid' must be specified when initializing EEPROM for the first time.\n");
	printf("Valid values are: 0x4242 (N310, N320), 0x4240 (N300), 0xe320 (E320).\n");
}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep, *ep_stored;
	u16 rev;
	u16 mcu_compat;
	u16 dt_compat;
	u16 rev_compat;
	long pid = 0;

	if (argc < 6 || argc > 10) {
		usage(argv);
		return EXIT_FAILURE;
	}
	rev = atoi(argv[2]);

	/* First, we try reading the existing EEPROM contents. This may fail,
	 * and that's fine. But if it works, we can use it to fill in missing
	 * defaults based on the existing data.
	 */
	ep_stored = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	if (!ep_stored) {
		printf("-- EEPROM is either uninitialized or corrupt. Initializing all fields...\n");
	}

	if (argc >= 7) {
		pid = strtol(argv[6], NULL, 0);
	} else if (ep_stored) {
		pid = ntohs(ep_stored->pid);
	} else {
		printf("-- ERROR: Cannot derive PID from existing EEPROM or command line!\n");
		return EXIT_FAILURE;
	}
	if (pid != N310_PID && pid != N300_PID && pid != E320_PID) {
		printf("Invalid PID: %lX\n", pid);
		return EXIT_FAILURE;
	}

	/* If no MCU or DT compat specified, either use existing values, or
	 * derive based on rules defined at the top of this file.
	 */
	if (argc >= 8) {
		dt_compat = strtol(argv[7], NULL, 0);
		printf("dt_compat=%u\n", dt_compat);
	} else if (ep_stored) {
		dt_compat = ntohs(ep_stored->dt_compat);
		printf("dt_compat=%u\n", dt_compat);
	} else {
		dt_compat = derive_dt_compat(rev, pid);
	}

	if (argc >= 9) {
		mcu_compat = strtol(argv[8], NULL, 0);
		printf("mcu_compat=%u\n", mcu_compat);
	} else if (ep_stored) {
		mcu_compat = ntohs(ep_stored->mcu_compat);
		printf("mcu_compat=%u\n", mcu_compat);
	} else {
		mcu_compat = derive_mcu_compat(rev, pid);
	}

	if (argc >= 10) {
		rev_compat = strtol(argv[9], NULL, 0);
		printf("rev_compat=%u\n", rev_compat);
	} else if (ep_stored && rev == ep_stored->rev) {
		rev_compat = ntohs(ep_stored->rev_compat);
		printf("rev_compat=%u\n", rev_compat);
	} else {
		rev_compat = derive_rev_compat(rev, pid);
	}

	if (ep_stored)
		free(ep_stored);

	ep = usrp_sulfur_eeprom_new(NULL, (u16) pid, rev, argv[1],
			            argv[3], argv[4], argv[5], dt_compat,
				    mcu_compat, rev_compat);

	usrp_sulfur_eeprom_print(ep);
	usrp_sulfur_eeprom_to_i2c(ep, "/dev/i2c-2");
	free(ep);

	printf("-- Reading back \n");
	ep_stored = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	usrp_sulfur_eeprom_print(ep_stored);
	free(ep_stored);
}
