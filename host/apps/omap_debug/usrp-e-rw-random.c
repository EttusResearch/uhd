#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include "usrp_e.h"

// max length #define PKT_DATA_LENGTH 1014
static int packet_data_length;

struct pkt {
	int checksum;
	int seq_num;
	int len;
	short data[];
};

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i < p->len; i++)
		sum += p->data[i];

	sum += p->seq_num;

	return sum;
}

int randN(int n)
{
	long tmp;

	tmp = rand()  % n;

	return tmp;
}

static void *read_thread(void *threadid)
{
	int cnt, prev_seq_num;
	struct usrp_transfer_frame *rx_data;
	struct pkt *p;

	printf("Greetings from the reading thread!\n");

	// IMPORTANT: must assume max length packet from fpga
	rx_data = malloc(sizeof(struct usrp_transfer_frame) + sizeof(struct pkt) + (1014 * 2));
	rx_data = malloc(2048);
	p = (struct pkt *) ((void *)rx_data + offsetof(struct usrp_transfer_frame, buf));
	//p = &(rx_data->buf[0]);
	printf("Address of rx_data = %p, p = %p\n", rx_data, p);
	printf("offsetof = %d\n", offsetof(struct usrp_transfer_frame, buf));
	printf("sizeof rx data = %d\n", sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));

	prev_seq_num = 0;

	while (1) {

		cnt = read(fp, rx_data, 2048);
//		printf("Packet received, flags = %X, len = %d\n", rx_data->flags, rx_data->len);
//		printf("p->seq_num = %d\n", p->seq_num);

		if (p->seq_num != prev_seq_num + 1)
			printf("Sequence number fail, current = %d, previous = %d\n",
				p->seq_num, prev_seq_num);
		prev_seq_num = p->seq_num;

		if (calc_checksum(p) != p->checksum)
			printf("Checksum fail packet = %d, expected = %d\n",
				calc_checksum(p), p->checksum);
		printf(".");
		fflush(stdout);
//		printf("\n");
	}

}

static void *write_thread(void *threadid)
{
	int seq_number, i, cnt, pkt_cnt;
	struct usrp_transfer_frame *tx_data;
	struct pkt *p;

	printf("Greetings from the write thread!\n");

	// Allocate max length buffer for frame
	tx_data = malloc(2048);
	p = (struct pkt *) ((void *)tx_data + offsetof(struct usrp_transfer_frame, buf));
	printf("Address of tx_data = %p, p = %p\n", tx_data, p);

	printf("sizeof rp_transfer_frame = %d, sizeof pkt = %d\n", sizeof(struct usrp_transfer_frame), sizeof(struct pkt));

	for (i=0; i < 1014; i++)
//		p->data[i] = random() >> 16;
		p->data[i] = i;

	tx_data->flags = 0xdeadbeef;
	tx_data->len = 8 + packet_data_length * 2;

	printf("tx_data->len = %d\n", tx_data->len);

	seq_number = 1;

	while (1) {
		pkt_cnt = randN(16);
		for (i = 0; i < pkt_cnt; i++) {
			p->seq_num = seq_number++;
			p->len = randN(1013) + 1;
			p->checksum = calc_checksum(p);
			tx_data->len = 12 + p->len * 2;
			cnt = write(fp, tx_data, tx_data->len + 8);
		}
		sleep(random() >> 31);
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

	sleep(1000000000);

	printf("Done sleeping\n");
}
