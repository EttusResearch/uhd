//
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <stdio.h>
#include <stdlib.h>

#include "tlv_eeprom.h"
#include "tlv_eeprom_io.h"
#include "usrp_eeprom.h"
#include "eeprom-pids.h"

int main(int argc, char **argv)
{
	struct tlv_eeprom *eeprom;
	const struct usrp_eeprom_module_info *module_info;
	const struct usrp_eeprom_board_info *board_info;
	const struct pid_info *pid_info;
	unsigned int pid, rev;
	const char *serial;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <eeprom>\n", argv[0]);
		return 1;
	}

	eeprom = tlv_eeprom_read_from_file(argv[1]);
	if (tlv_eeprom_validate(eeprom, USRP_EEPROM_MAGIC)) {
		fprintf(stderr, "failed to validate eeprom\n");
		return 1;
	}

	module_info = tlv_lookup(eeprom->tlv, eeprom->size,
				 USRP_EEPROM_MODULE_INFO_TAG);
	board_info = tlv_lookup(eeprom->tlv, eeprom->size,
				USRP_EEPROM_BOARD_INFO_TAG);

	if (module_info) {
		serial = module_info->serial;
		pid = module_info->pid;
		rev = module_info->rev;
	} else if (board_info) {
		serial = board_info->serial;
		pid = board_info->pid;
		rev = board_info->rev;
	} else  {
		fprintf(stderr, "couldn't find module_info or board_info\n");
		return 1;
	}

	pid_info = get_info_from_pid(pid);

	if (pid_info && pid_info->name) {
		printf("product=ni-%s-rev%u\n", pid_info->name, rev + pid_info->rev_offset);
	} else if (pid_info && pid_info->description) {
		printf("product=ni-(%s)-rev%u\n", pid_info->description, rev + pid_info->rev_offset);
	} else {
		printf("product=unknown-(%04x)\n", pid);
	}
	printf("serial=%s\n", serial);

	free(eeprom);
	return 0;
}
