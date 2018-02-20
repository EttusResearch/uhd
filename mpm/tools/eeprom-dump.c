//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
	struct usrp_sulfur_eeprom *ep;

	ep = usrp_sulfur_eeprom_from_file(NVMEM_PATH_MB);
	if (!ep)
		return EXIT_FAILURE;
	usrp_sulfur_eeprom_print(ep);
	free(ep);

	return 0;
}
