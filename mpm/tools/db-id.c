//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <stdio.h>
#include <stdlib.h>
#include "eeprom.h"

static void usrp_sulfur_db_eeprom_print_id(struct usrp_sulfur_db_eeprom *ep)
{
	int rev;

	if (ntohl(ep->version) == 1)
		rev = ntohs(ep->rev.v1_rev);
	else
		rev = ep->rev.v2_rev.rev;

	if (ntohs(ep->pid) == 0x150)
		printf("product=ni,magnesium-rev%x\n", rev + 1);
	else if (ntohs(ep->pid) == 0x180)
		printf("product=ni,eiscat-rev%x\n", rev + 1);
	else
		printf("product=unknown-(%04x)\n", ep->pid);

	printf("serial=%s\n", ep->serial);
}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_db_eeprom *ep;

	ep = usrp_sulfur_db_eeprom_from_file(NVMEM_PATH_SLOT_A);
	if (ep) {
		printf("Slot-0\n");
		usrp_sulfur_db_eeprom_print_id(ep);
		free(ep);
	}

	ep = usrp_sulfur_db_eeprom_from_file(NVMEM_PATH_SLOT_B);
	if (ep) {
		printf("Slot-1\n");
		usrp_sulfur_db_eeprom_print_id(ep);
		free(ep);
	}

	return EXIT_SUCCESS;
}

