#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include "usrp_e.h"

#define PKT_DATA_LENGTH 30
// max length #define PKT_DATA_LENGTH 1016

struct pkt {
	int checksum;
	int seq_num;
	short data[PKT_DATA_LENGTH];
};

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i<PKT_DATA_LENGTH; i++)
		sum += p->data[i];

	sum += p->seq_num;

	return sum;
}

static void *read_thread(void *threadid)
{
	int cnt, prev_seq_num;
	struct usrp_transfer_frame *rx_data;
	struct pkt *p;

	printf("Greetings from the reading thread!\n");

	rx_data = malloc(sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));
	p = (struct pkt *) ((void *)rx_data + offsetof(struct usrp_transfer_frame, buf));
	//p = &(rx_data->buf[0]);
	printf("Address of rx_data = %p, p = %p\n", rx_data, p);
	printf("offsetof = %d\n", offsetof(struct usrp_transfer_frame, buf));
	printf("sizeof rx data = %X\n", sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));

	prev_seq_num = 0;

	while (1) {

		cnt = read(fp, rx_data, 2048);
		printf("Packet received, flags = %X, len = %X\n", rx_data->flags, rx_data->len);
		printf("p->seq_num = %d\n", p->seq_num);

		if (p->seq_num != prev_seq_num + 1)
			printf("Sequence number fail, current = %X, previous = %X\n",
				p->seq_num, prev_seq_num);
		prev_seq_num = p->seq_num;

		if (calc_checksum(p) != p->checksum)
			printf("Checksum fail packet = %X, expected = %X\n",
				calc_checksum(p), p->checksum);
		printf("\n");
	}

}

static void *write_thread(void *threadid)
{
	int seq_number, i, cnt;
	struct usrp_transfer_frame *tx_data;
	struct pkt *p;

	printf("Greetings from the write thread!\n");

	tx_data = malloc(sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));
	p = (struct pkt *) ((void *)tx_data + offsetof(struct usrp_transfer_frame, buf));
	printf("Address of tx_data = %p, p = %p\n", tx_data, p);

	printf("sizeof tx data = %X\n", sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));

	for (i=0; i<PKT_DATA_LENGTH; i++)
//		p->data[i] = random() >> 16;
		p->data[i] = i;

	tx_data->flags = 0xdeadbeef;
	tx_data->len = 8 + PKT_DATA_LENGTH * 2;

	seq_number = 1;

	while (1) {
		p->seq_num = seq_number++;
		p->checksum = calc_checksum(p);
		cnt = write(fp, tx_data, 2048);
		sleep(1);
	}
}


int main(int argc, char *argv[])
{
	pthread_t tx, rx;
	long int t;

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (pthread_create(&rx, NULL, read_thread, (void *) t)) {
		printf("Failed to create rx thread\n");
		exit(-1);
	}

	sleep(1);

	if (pthread_create(&tx, NULL, write_thread, (void *) t)) {
		printf("Failed to create tx thread\n");
		exit(-1);
	}

	sleep(10000);

	printf("Done sleeping\n");
}
