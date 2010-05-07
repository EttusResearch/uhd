#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "usrp_e.h"

// Usage: usrp_e_i2c w address data0 data1 data 2 ....
// Usage: usrp_e_i2c r address count

int main(int argc, char *argv[])
{
	int fp, ret, i, tmp;
	struct usrp_e_i2c *i2c_msg;
	int direction, address, count;

	if (argc < 3) {
		printf("Usage: usrp-e-i2c w address data0 data1 data2 ...\n");
		printf("Usage: usrp-e-i2c r address count\n");
		printf("All addresses and data in hex.\n");
		exit(-1);
	}

	if (strcmp(argv[1], "r") == 0) {
		direction = 0;
	} else if (strcmp(argv[1], "w") == 0) {
		direction = 1;
	} else {
		return -1;
	}

	sscanf(argv[2], "%X", &address);
	printf("Address = %X\n", address);

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);
	if (fp < 0) {
		perror("Open failed");
		return -1;
	}

//	sleep(1);

	if (direction) {
		count = argc - 3;
	} else {
		sscanf(argv[3], "%X", &count);
	}
	printf("Count = %X\n", count);

	i2c_msg = malloc(sizeof(i2c_msg) + count * sizeof(char));

	i2c_msg->addr = address;
	i2c_msg->len = count;

	for (i = 0; i < count; i++) {
		i2c_msg->data[i] = i;
	}

	if (direction) {
		// Write

		for (i=0; i<count; i++) {
			sscanf(argv[3+i], "%X", &tmp);
			i2c_msg->data[i] = tmp;
		}

		ret = ioctl(fp, USRP_E_I2C_WRITE, i2c_msg);
		printf("Return value from i2c_write ioctl: %d\n", ret);
	} else {
		// Read

		ret = ioctl(fp, USRP_E_I2C_READ, i2c_msg);
		printf("Return value from i2c_read ioctl: %d\n", ret);

		printf("Ioctl: %d Data read :", ret);
		for (i=0; i<count; i++) {
			printf(" %X", i2c_msg->data[i]);
		}
		printf("\n");
			
	}
	return 0;
}
