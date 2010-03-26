#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "usrp_e.h"

// Usage: usrp_e_uart <string>

#define UART_WRITE_ADDR ((1<<6) + 16) 

int main(int argc, char *argv[])
{
	int fp, i, ret;
	struct usrp_e_ctl16 d;
	char *str = argv[1];

	if (argc < 2) {
		printf("Usage: usrp_e_uart <string>n");
		exit(-1);
	}

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	for (i=0; i<strlen(str); i++) {
		d.offset = UART_WRITE_ADDR;
		d.count = 1;
		d.buf[0] = str[i];
		ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
		printf("Wrote %X, to %X, ret = %d\n", d.buf[0], d.offset, ret);
	}

	return 0;
}
