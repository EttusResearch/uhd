#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "usrp1_e.h"

// Usage: usrp_e_i2c w address data0 data1 data 2 ....
// Usage: usrp_e_i2c r address count

int main(int argc, char *argv[])
{
	int fp, ret, i;
	struct usrp_e_i2c *i2c_msg;
	int direction, address, count;

	if (argc < 3) {
		printf("Usage: usrp1_e_i2c w address data0 data1 data2 ...\n");
		printf("Usage: usrp1_e_i2c r address count\n");
		exit(-1);
	}

	if (strcmp(argv[1], "r") == 0) {
		direction = 0;
	} else if (strcmp(argv[1], "w") == 0) {
		direction = 1;
	} else {
		return -1;
	}

	address = atoi(argv[2]);

	fp = open("/dev/usrp1_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (direction) {
		count = argc - 2;
	} else {
		count = atoi(argv[3]);
	}

	i2c_msg = malloc(sizeof(i2c_msg) + count * sizeof(char));

	i2c_msg->addr = address;
	i2c_msg->len = count;

	if (direction) {
		// Write

		for (i=0; i<count; i++) {
			i2c_msg->data[i] = atoi(argv[3+i]);
		}

		ret = ioctl(fp, USRP_E_I2C_WRITE, i2c_msg);
		printf("Return value from i2c_write ioctl: %d\n", ret);
	} else {
		// Read

		ret = ioctl(fp, USRP_E_I2C_READ, i2c_msg);

		printf("Ioctl: %d Data read :", ret);
		for (i=0; i<count; i++) {
			printf(" %X", i2c_msg->data[i]);
		}
		printf("/n");
			
	}
	return 0;
}
