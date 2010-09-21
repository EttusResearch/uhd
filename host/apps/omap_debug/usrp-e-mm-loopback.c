#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/mman.h>
#include <poll.h>
#include "usrp_e.h"

// max length #define PKT_DATA_LENGTH 1016
static int packet_data_length;
static int error;

struct pkt {
	int len;
	int checksum;
	int seq_num;
	short data[1024-6];
};

struct ring_buffer_info (*rxi)[];
struct ring_buffer_info (*txi)[];
struct pkt (*rx_buf)[200];
struct pkt (*tx_buf)[200];

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i < p->len; i++)
		sum += p->data[i];

	sum += p->seq_num;
	sum += p->len;

	return sum;
}

static void *read_thread(void *threadid)
{
	int cnt, prev_seq_num, pkt_count, seq_num_failure;
	struct pkt *p;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;
	int rb_read;

	printf("Greetings from the reading thread!\n");
	printf("sizeof pkt = %d\n", sizeof(struct pkt));

	rb_read = 0;

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

	prev_seq_num = 0;
	pkt_count = 0;
	seq_num_failure = 0;

	while (1) {

		if (!((*rxi)[rb_read].flags & RB_USER)) {
//			printf("Waiting for data\n");
			struct pollfd pfd;
			pfd.fd = fp;
			pfd.events = POLLIN;
			ssize_t ret = poll(&pfd, 1, -1);
		}

//		printf("pkt received, rb_read = %d\n", rb_read);

		cnt = (*rxi)[rb_read].len;
		p = &(*rx_buf)[rb_read];

//		cnt = read(fp, rx_data, 2048);
//		if (cnt < 0)
//			printf("Error returned from read: %d, sequence number = %d\n", cnt, p->seq_num);

//		printf("p = %X, p->seq_num = %d p->len = %d\n", p, p->seq_num, p->len);


		pkt_count++;

		if (p->seq_num != prev_seq_num + 1) {
			printf("Sequence number fail, current = %d, previous = %d, pkt_count = %d\n",
				p->seq_num, prev_seq_num, pkt_count);
			printf("pkt received, rb_read = %d\n", rb_read);
			printf("p = %X, p->seq_num = %d p->len = %d\n", p, p->seq_num, p->len);

			seq_num_failure ++;
			if (seq_num_failure > 2)
				error = 1;
		}

		prev_seq_num = p->seq_num;

		if (calc_checksum(p) != p->checksum) {
			printf("Checksum fail packet = %X, expected = %X, pkt_count = %d\n",
				calc_checksum(p), p->checksum, pkt_count);
			error = 1;
		}

		(*rxi)[rb_read].flags = RB_KERNEL;

		rb_read++;
		if (rb_read == 100)
			rb_read = 0;

		bytes_transfered += cnt;

		if (bytes_transfered > (100 * 1000000)) {
			gettimeofday(&finish_time, NULL);
			elapsed_seconds = finish_time.tv_sec - start_time.tv_sec;

			printf("RX data transfer rate = %f K Samples/second\n",
				(float) bytes_transfered / (float) elapsed_seconds / 4000);


			start_time = finish_time;
			bytes_transfered = 0;
		}


//		printf(".");
//		fflush(stdout);
//		printf("\n");
	}

}

static void *write_thread(void *threadid)
{
	int seq_number, i, cnt, rb_write;
	void *tx_data;
	struct pkt *p;

	printf("Greetings from the write thread!\n");

	tx_data = malloc(2048);
	p = (struct pkt *) ((void *)tx_data);

	for (i=0; i < packet_data_length; i++)
//		p->data[i] = random() >> 16;
		p->data[i] = i;

	seq_number = 1;
	rb_write = 0;

	while (1) {
		p->seq_num = seq_number++;

		if (packet_data_length > 0)
			p->len = packet_data_length;
		else
			p->len = (random() & 0x1ff) + (1004 - 512);

		p->checksum = calc_checksum(p);

		if (!((*txi)[rb_write].flags & RB_KERNEL)) {
//			printf("Waiting for space\n");
			struct pollfd pfd;
			pfd.fd = fp;
			pfd.events = POLLOUT;
			ssize_t ret = poll(&pfd, 1, -1);
		}

		memcpy(&(*tx_buf)[rb_write], tx_data, p->len * 2 + 12);

		(*txi)[rb_write].len = p->len * 2 + 12;
		(*txi)[rb_write].flags = RB_USER;

		rb_write++;
		if (rb_write == 100)
			rb_write = 0;

//		cnt = write(fp, tx_data, p->len * 2 + 12);
//		if (cnt < 0)
//			printf("Error returned from write: %d\n", cnt);
//		sleep(1);
	}
}


int main(int argc, char *argv[])
{
	pthread_t tx, rx;
	long int t;
	struct sched_param s = {
		.sched_priority = 1
	};
	struct usrp_e_ring_buffer_size_t rb_size;
	int ret, map_size, page_size;
	void *rb;

	if (argc < 2) {
		printf("%s data_size\n", argv[0]);
		return -1;
	}

	packet_data_length = atoi(argv[1]);

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	page_size = getpagesize();

	ret = ioctl(fp, USRP_E_GET_RB_INFO, &rb_size);

	map_size = (rb_size.num_pages_rx_flags + rb_size.num_pages_tx_flags) * page_size +
		(rb_size.num_rx_frames + rb_size.num_tx_frames) * (page_size >> 1);

	rb = mmap(0, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fp, 0);
	if (rb == MAP_FAILED) {
		perror("mmap failed");
		return -1;
	}

	printf("rb = %X\n", rb);

	rxi = rb;
	rx_buf = rb + (rb_size.num_pages_rx_flags * page_size);
	txi = rb +  (rb_size.num_pages_rx_flags * page_size) +
		(rb_size.num_rx_frames * page_size >> 1);
	tx_buf = rb +  (rb_size.num_pages_rx_flags * page_size) +
		(rb_size.num_rx_frames * page_size >> 1) +
		(rb_size.num_pages_tx_flags * page_size);

	printf("rxi = %X, rx_buf = %X, txi = %X, tx_buf = %X\n", rxi, rx_buf, txi, tx_buf);

	sched_setscheduler(0, SCHED_RR, &s);
	error = 0;

#if 1
	if (pthread_create(&rx, NULL, read_thread, (void *) t)) {
		printf("Failed to create rx thread\n");
		exit(-1);
	}

	sleep(1);
#endif

	if (pthread_create(&tx, NULL, write_thread, (void *) t)) {
		printf("Failed to create tx thread\n");
		exit(-1);
	}

//	while (!error)
		sleep(1000000000);

	printf("Done sleeping\n");
}
