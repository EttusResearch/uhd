#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

struct pkt {
	int checksum;
	int seq_num;
	short data[1020];
};

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i<1020; i++)
		sum += p->data[i];

	sum += p->seq_num;

	return sum;
}

static void *read_thread(void *threadid)
{
	int cnt, prev_seq_num;
	struct pkt rx_data;

	printf("Greetings from the reading thread!\n");

	prev_seq_num = 0;

	while (1) {
		cnt = read(fp, &rx_data, 2048);

		if (rx_data.seq_num != prev_seq_num + 1)
			printf("Sequence number fail, current = %d, previous = %d\n",
				rx_data.seq_num, prev_seq_num);
		prev_seq_num = rx_data.seq_num;

		if (calc_checksum(&rx_data) != rx_data.checksum)
			printf("Checksum fail packet = %d, expected = %d\n",
				calc_checksum(&rx_data), rx_data.checksum);
	}

}

static void *write_thread(void *threadid)
{
	int seq_number, i, cnt;
	struct pkt tx_data;

	printf("Greetings from the write thread!\n");

	for (i=0; i<1020; i++)
		tx_data.data[i] = random() >> 16;


	seq_number = 1;

	while (1) {
		tx_data.seq_num = seq_number++;
		tx_data.checksum = calc_checksum(&tx_data);
		cnt = write(fp, &tx_data, 2048);
	}
}


int main(int argc, char *argv[])
{
	int ret;
	pthread_t tx, rx;
	long int t;

	fp = open("/dev/usrp1_e0", O_RDWR);
	printf("fp = %d\n", fp);

	if (pthread_create(&rx, NULL, read_thread, (void *) t)) {
		printf("Failed to create rx thread\n");
		exit(-1);
	}

	if (pthread_create(&tx, NULL, write_thread, (void *) t)) {
		printf("Failed to create tx thread\n");
		exit(-1);
	}

	sleep(10000);

	printf("Done sleeping\n");
}
