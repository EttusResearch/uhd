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
	int test;

	test = 0;
	if (argc < 2) {
		printf("%s 0|1|off\n", argv[0]);
	}

        fp = open("/dev/usrp_e0", O_RDWR);
        printf("fp = %d\n", fp);
	if (fp < 0) {
		perror("Open failed");
		return -1;
	}

	if (strcmp(argv[1], "0") == 0) {
		printf("Selected 0 based on %s\n", argv[1]);
		write_reg(UE_REG_GPIO_TX_DDR, 0xFFFF);
		write_reg(UE_REG_GPIO_RX_DDR, 0xFFFF);
		write_reg(UE_REG_GPIO_TX_SEL, 0x0);
		write_reg(UE_REG_GPIO_RX_SEL, 0x0);
		write_reg(UE_REG_GPIO_TX_DBG, 0xFFFF);
		write_reg(UE_REG_GPIO_RX_DBG, 0xFFFF);
	} else if (strcmp(argv[1], "1") == 0) {
		printf("Selected 1 based on %s\n", argv[1]);
		write_reg(UE_REG_GPIO_TX_DDR, 0xFFFF);
		write_reg(UE_REG_GPIO_RX_DDR, 0xFFFF);
		write_reg(UE_REG_GPIO_TX_SEL, 0xFFFF);
		write_reg(UE_REG_GPIO_RX_SEL, 0xFFFF);
		write_reg(UE_REG_GPIO_TX_DBG, 0xFFFF);
		write_reg(UE_REG_GPIO_RX_DBG, 0xFFFF);
	} else {
		printf("Selected off based on %s\n", argv[1]);
		write_reg(UE_REG_GPIO_TX_DDR, 0x0);
		write_reg(UE_REG_GPIO_RX_DDR, 0x0);
	}

	return 0;
}
