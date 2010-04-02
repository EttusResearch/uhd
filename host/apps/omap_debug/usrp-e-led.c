#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "usrp_e.h"
#include "usrp_e_regs.hpp"

// Usage: usrp_e_uart <string>


int main(int argc, char *argv[])
{
	int fp, i, ret;
	struct usrp_e_ctl16 d;

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	d.offset = UE_REG_MISC_BASE;
	d.count = 1;

	while (1) {
		for (i=0; i<8; i++) {
			d.buf[0] = i;
			ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
			sleep(1);
		}
	}

	return 0;
}
