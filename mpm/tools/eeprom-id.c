//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include <stdio.h>
#include <stdlib.h>
#include "eeprom.h"

static void usrp_sulfur_eeprom_print_id(struct usrp_sulfur_eeprom *ep)
{
	if (ep->pid == 0x4242)
		printf("product=ni,n310-rev%x\n", ntohs(ep->rev)+1);
	else
		printf("product=unknown-(%04x)\n", ntohs(ep->pid));

	printf("serial=%s\n", ep->serial);
}

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep;

	ep = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	if (!ep)
		return EXIT_FAILURE;

	usrp_sulfur_eeprom_print_id(ep);

	free(ep);
}
