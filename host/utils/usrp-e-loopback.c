#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/mman.h>
#include <linux/usrp_e.h>

#define MAX_PACKET_SIZE 1016
static int packet_data_length;
static int error;

struct pkt {
	int len;
	int checksum;
	int seq_num;
	short data[];
};

static int length_array[2048];
static int length_array_tail = 0;
static int length_array_head = 0;

pthread_mutex_t length_array_mutex; //gotta lock the index to keep it from getting hosed

//yes this is a circular buffer that does not check empty
//no i don't want to hear about it
void push_length_array(int length) {
	pthread_mutex_lock(&length_array_mutex);
	if(length_array_tail > 2047) length_array_tail = 0;
	length_array[length_array_tail++] = length;
	pthread_mutex_unlock(&length_array_mutex);
}

int pop_length_array(void) {
	int retval;
	pthread_mutex_lock(&length_array_mutex);
	if(length_array_head > 2047) length_array_head = 0;
	retval = length_array[length_array_head++];
	pthread_mutex_unlock(&length_array_mutex);
	return retval;
}

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i < p->len; i++)
		sum ^= p->data[i];

	sum ^= p->seq_num;
	sum ^= p->len;

	return sum;
}

static void *read_thread(void *threadid)
{
	char *rx_data;
	int cnt, prev_seq_num, pkt_count, seq_num_failure;
	struct pkt *p;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;
	int expected_count;

	printf("Greetings from the reading thread!\n");

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

	// IMPORTANT: must assume max length packet from fpga
	rx_data = malloc(2048);
	p = (struct pkt *) ((void *)rx_data);

	prev_seq_num = 0;
	pkt_count = 0;
	seq_num_failure = 0;

	while (1) {

		cnt = read(fp, rx_data, 2048);
		if (cnt < 0)
			printf("Error returned from read: %d, sequence number = %d\n", cnt, p->seq_num);

//		printf("p->seq_num = %d\n", p->seq_num);


		pkt_count++;

		if (p->seq_num != prev_seq_num + 1) {
			printf("Sequence number fail, current = %d, previous = %d, pkt_count = %d\n",
				p->seq_num, prev_seq_num, pkt_count);

			seq_num_failure ++;
			if (seq_num_failure > 2)
				error = 1;
		}

		expected_count = pop_length_array()*2+12;
		if(cnt != expected_count) {
			printf("Received %d bytes, expected %d\n", cnt, expected_count);
		}

		prev_seq_num = p->seq_num;

		if (calc_checksum(p) != p->checksum) {
			printf("Checksum fail packet = %X, expected = %X, pkt_count = %d\n",
				calc_checksum(p), p->checksum, pkt_count);
			error = 1;
		}

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
	int seq_number, i, cnt;
	void *tx_data;
	struct pkt *p;

	printf("Greetings from the write thread!\n");

	tx_data = malloc(2048);
	p = (struct pkt *) ((void *)tx_data);

	for (i=0; i < packet_data_length; i++)
//		p->data[i] = random() >> 16;
		p->data[i] = i;

	seq_number = 1;

	while (1) {
		p->seq_num = seq_number++;

		if (packet_data_length > 0)
			p->len = packet_data_length;
		else
			p->len = (random()<<1 & 0x1ff) + (1004 - 512);

		push_length_array(p->len);

		p->checksum = calc_checksum(p);

		cnt = write(fp, tx_data, p->len * 2 + 12);
		if (cnt < 0)
			printf("Error returned from write: %d\n", cnt);
//		sleep(1);
	}
}


int main(int argc, char *argv[])
{
	pthread_t tx, rx;
	pthread_mutex_init(&length_array_mutex, 0);
	long int t;
	struct sched_param s = {
		.sched_priority = 1
	};
	void *rb;
	struct usrp_transfer_frame *tx_rb, *rx_rb;

	if (argc < 2) {
		printf("%s data_size\n", argv[0]);
		return -1;
	}

	packet_data_length = atoi(argv[1]);
	if(packet_data_length > MAX_PACKET_SIZE) {
		printf("Packet size must be smaller than %i\n", MAX_PACKET_SIZE);
		exit(-1);
	}

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	rb = mmap(0, 202 * 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fp, 0);
	if (!rb) {
		printf("mmap failed\n");
		exit;
	}


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
