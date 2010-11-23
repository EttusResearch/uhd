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

#define PB1 (1<<8)
#define PB2 (1<<9)
#define PB3 (1<<10)
#define P1 (0)
#define P2 (0xFF)
#define P3 (0xAA)
#define P4 (0x55)

int main(int argc, char *argv[])
{
	int fp, ret;
	struct usrp_e_ctl16 d;
	int pb1=0, pb2=0, pb3=0, p1=0, p2=0, p3=0, p4=0;

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	d.offset = UE_REG_MISC_SW;
	d.count = 1;

	do {
		ret = ioctl(fp, USRP_E_READ_CTL16, &d);
		if (d.buf[0] & PB1) {
			pb1 = 1;
			printf("Pushbutton 1 hit\n");
		}

		if (d.buf[0] & PB2) {
			pb2 = 1;
			printf("Pushbutton 2 hit\n");
		}

		if (d.buf[0] & PB3) {
			pb3 = 1;
			printf("Pushbutton 3 hit\n");
		}

		sleep(1);

	} while (!(pb1 && pb2 && pb3));

	return 0;
}
