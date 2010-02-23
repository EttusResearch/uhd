#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/ioctl.h>

#include "usrp1_e.h"

// Usage: usrp1_e_ctl w|r offset number_of_values val1 val2 ....

int main(int argc, char *argv[])
{
	int fp, i, cnt, ret;
	struct usrp1_e_ctl *ctl_data;

	if (argc < 4) {
		printf("Usage: usrp1_e_ctl w|r offset number_of_values val1 val2 ....\n");
		exit(-1);
	}

	cnt = atoi(argv[3]);

	ctl_data = malloc(sizeof(struct usrp1_e_ctl) + cnt*2);

	ctl_data->offset = atoi(argv[2]);
	ctl_data->count  = cnt;

	printf("Sizeof usrp1_e_ctl struct = %d\n", sizeof(struct usrp1_e_ctl));

	fp = open("/dev/usrp1_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (*argv[1] == 'w') {
		for (i=0; i<cnt; i++)
			ctl_data->buf[i] = atoi(argv[4+i]);

		ret = ioctl(fp, USRP1_E_WRITE_CTL, ctl_data);
		printf("Return value from write ioctl = %d\n", ret);
	}

	if (*argv[1] == 'r') {
		ret = ioctl(fp, USRP1_E_READ_CTL, ctl_data);
		printf("Return value from write ioctl = %d\n", ret);

		for (i=0; i<ctl_data->count; i++) {
			if (!(i%8))
				printf("\nData at %4d :", i);
			printf(" %5d", ctl_data->buf[i]);
		}
		printf("\n");
	}
}
