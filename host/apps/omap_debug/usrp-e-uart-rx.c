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
	int fp, ret;
	struct usrp_e_ctl16 d;
	__u16 clkdiv;

	if (argc == 0) {
		printf("Usage: usrp-e-uart-rx <opt clkdiv>\n");
		printf("clkdiv = 278 is 230.4k \n");
		printf("clkdiv = 556 is 115.2k \n");
		exit(-1);
	}

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (argc == 2) {
		clkdiv = atoi(argv[1]);
		d.offset = UE_REG_UART_CLKDIV;
		d.count = 1;
		d.buf[0] = clkdiv;
		ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
	}

	while(1) {
		d.offset = UE_REG_UART_RXLEVEL;
		d.count = 1;
		ret = ioctl(fp, USRP_E_READ_CTL16, &d);

		if (d.buf[0] > 0) {
			d.offset = UE_REG_UART_RXCHAR;
			d.count = 1;
			ret = ioctl(fp, USRP_E_READ_CTL16, &d);
			printf("%c", d.buf[0]);
			fflush(stdout);
		}
	}

	return 0;
}
