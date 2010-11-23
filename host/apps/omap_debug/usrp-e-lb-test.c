#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include "usrp_e.h"

// max length #define PKT_DATA_LENGTH 1016

int main(int argc, char *argv[])
{
	struct usrp_transfer_frame *tx_data, *rx_data;
	int i, fp, packet_data_length, cnt;
	struct usrp_e_ctl16 d;

	if (argc < 2) {
		printf("%s data_size (in bytes < 2040)\n", argv[0]);
		return -1;
	}

	packet_data_length = atoi(argv[1]);

	fp = open("/dev/usrp_e0", O_RDWR);

	d.offset = 14;
	d.count = 1;
	d.buf[0] = (1 << 13);
	ioctl(fp, USRP_E_WRITE_CTL16, &d);

	tx_data = malloc(2048);
	rx_data = malloc(2048);

	tx_data->status = 0;
	tx_data->len = sizeof(struct usrp_transfer_frame) + packet_data_length;

	while (1) {

		for (i = 0; i < packet_data_length; i++) {
			tx_data->buf[i] = random() >> 24;

		}

		cnt = write(fp, tx_data, 2048);
		cnt = read(fp, rx_data, 2048);

		if (tx_data->len != rx_data->len)
			printf("Bad frame length sent %d, read %d\n", tx_data->len, rx_data->len);

		for (i = 0; i < packet_data_length; i++) {
			if (tx_data->buf[i] != rx_data->buf[i])
				printf("Bad data at %d, sent %d, received %d\n", i, tx_data->buf[i], rx_data->buf[i]);
		}
		printf("---------------------------------------------------\n");
		sleep(1);
	}
}
