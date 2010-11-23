#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "usrp_e.h"
#include "usrp_e_regs.hpp"

// Usage: usrp_e_uart <string>


int main(int argc, char *argv[])
{
	int fp, i, ret;
	struct usrp_e_ctl16 d;
	char *str = argv[1];
	__u16 clkdiv;

	if (argc < 2) {
		printf("Usage: usrp_e_uart <string> <opt clkdiv>\n");
		printf("clkdiv = 278 is 230.4k \n");
		printf("clkdiv = 556 is 115.2k \n");
		exit(-1);
	}

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (argc == 3) {
		clkdiv = atoi(argv[2]);
		d.offset = UE_REG_UART_CLKDIV;
		d.count = 1;
		d.buf[0] = clkdiv;
		ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
	}

	for (i=0; i<strlen(str); i++) {
		d.offset = UE_REG_UART_TXCHAR;
		d.count = 1;
		d.buf[0] = str[i];
		ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
		printf("Wrote %X, to %X, ret = %d\n", d.buf[0], d.offset, ret);
	}

	return 0;
}
