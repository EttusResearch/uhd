#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "usrp_e.h"
#include "usrp_e_regs.hpp"

// Usage: usrp_e_gpio <string>

static int fp;

static int read_reg(__u16 reg)
{
	int ret;
	struct usrp_e_ctl16 d;

	d.offset = reg;
	d.count = 1;
	ret = ioctl(fp, USRP_E_READ_CTL16, &d);
	return d.buf[0];
}

static void write_reg(__u16 reg, __u16 val)
{
	int ret;
	struct usrp_e_ctl16 d;

	d.offset = reg;
	d.count = 1;
	d.buf[0] = val;
	ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
}

int main(int argc, char *argv[])
{
	int i, test, data_in;

	test = 0;
	if (argc > 1)
		test = 1;

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	write_reg(UE_REG_GPIO_TX_DDR, 0x0);
	write_reg(UE_REG_GPIO_RX_DDR, 0xFFFF);

	for (i=0; i < 16; i++) {
		write_reg(UE_REG_GPIO_RX_IO, 1 << i);
		sleep(1);
		if (test) {
			data_in = read_reg(UE_REG_GPIO_TX_IO);
			if (data_in != (1 << i))
				printf("Read failed, wrote: %X read: %X\n", \
					1 << i, data_in);
		}
	}

	write_reg(UE_REG_GPIO_RX_DDR, 0x0);
	write_reg(UE_REG_GPIO_TX_DDR, 0xFFFF);

	sleep(1);

	for (i=0; i < 16; i++) {
		write_reg(UE_REG_GPIO_TX_IO, 1 << i);
		sleep(1);
		if (test) {
			data_in = read_reg(UE_REG_GPIO_RX_IO);
			if (data_in != (1 << i))
				printf("Read failed, wrote: %X read: %X\n", \
					1 << i, data_in);
		}
	}

	write_reg(UE_REG_GPIO_RX_DDR, 0x0);
	write_reg(UE_REG_GPIO_TX_DDR, 0x0);

	return 0;
}
