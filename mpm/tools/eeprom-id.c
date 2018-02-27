//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "eeprom.h"

#define N310_PID 0x4242
#define N300_PID 0x4240

static void usrp_sulfur_eeprom_print_id(struct usrp_sulfur_eeprom *ep)
{
	if (ntohs(ep->pid) == N310_PID)
		printf("product=ni,n310-rev%x\n", ntohs(ep->rev)+1);
	else if (ntohs(ep->pid) == N300_PID)
		printf("product=ni,n300-rev%x\n", ntohs(ep->rev)+1);
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
