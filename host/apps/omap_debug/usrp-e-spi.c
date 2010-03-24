#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "usrp1_e.h"

// Usage: usrp1_e_spi w|rb slave data

int main(int argc, char *argv[])
{
	int fp, slave, data, ret;
	struct usrp_e_spi spi_dat;

	if (argc < 4) {
		printf("Usage: usrp1_e_spi w|rb slave data\n");
		exit(-1);
	}

	slave = atoi(argv[2]);
	data = atoi(argv[3]);

	fp = open("/dev/usrp1_e0", O_RDWR);
	printf("fp = %d\n", fp);

	spi_dat.slave = slave;
	spi_dat.data = data;
	spi_dat.length = 2;
	spi_dat.flags = 0;

	if (*argv[1] == 'r') {
		spi_dat.readback = 1;
		ret = ioctl(fp, USRP_E_SPI, &spi_dat);
		printf("Data returned = %d\n", ret);
	} else {
		spi_dat.readback = 0;
		ioctl(fp, USRP_E_SPI, &spi_dat);
	}

	return 0;
}
