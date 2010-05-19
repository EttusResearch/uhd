#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include "usrp_e.h"

// max length #define PKT_DATA_LENGTH 1016
static int packet_data_length;

struct pkt {
	int checksum;
	int seq_num;
	short data[];
};

static int fp;

static int calc_checksum(struct pkt *p)
{
	int i, sum;

	i = 0;
	sum = 0;

	for (i=0; i < packet_data_length; i++)
		sum += p->data[i];

	sum += p->seq_num;

	return sum;
}

static void *read_thread(void *threadid)
{
	int cnt, prev_seq_num;
	struct usrp_transfer_frame *rx_data;
	struct pkt *p;
	int rx_pkt_cnt;
	int i;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;

	printf("Greetings from the reading thread!\n");

	// IMPORTANT: must assume max length packet from fpga
	rx_data = malloc(sizeof(struct usrp_transfer_frame) + sizeof(struct pkt) + (1016 * 2));
	p = (struct pkt *) ((void *)rx_data + offsetof(struct usrp_transfer_frame, buf));
	//p = &(rx_data->buf[0]);
	printf("Address of rx_data = %p, p = %p\n", rx_data, p);
	printf("offsetof = %d\n", offsetof(struct usrp_transfer_frame, buf));
	printf("sizeof rx data = %d\n", sizeof(struct usrp_transfer_frame) + sizeof(struct pkt));

	prev_seq_num = 0;

	rx_pkt_cnt = 0;

	while (1) {

		cnt = read(fp, rx_data, 2048);
		if (cnt < 0)
			printf("Error returned from read: %d\n", cnt);
		rx_pkt_cnt++;

#if 0
		if (rx_pkt_cnt  == 512) {
			printf(".");
			fflush(stdout);
			rx_pkt_cnt = 0;
		}
#endif

		if (rx_data->flags & RB_OVERRUN)
			printf("O");

		bytes_transfered += rx_data->len;

		if (bytes_transfered > (100 * 1000000)) {
			gettimeofday(&finish_time, NULL);
			elapsed_seconds = finish_time.tv_sec - start_time.tv_sec;

			printf("RX data transfer rate = %f K Bps\n",
				(float) bytes_transfered / (float) elapsed_seconds / 1000);


			start_time = finish_time;
			bytes_transfered = 0;
                }
	}

}

static void *write_thread(void *threadid)
{
	int seq_number, i, cnt, tx_pkt_cnt;
	struct usrp_transfer_frame *tx_data;
	struct pkt *p;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;

	printf("Greetings from the write thread!\n");

	tx_data = malloc(sizeof(struct usrp_transfer_frame) + sizeof(struct pkt) + (packet_data_length * 2));
	p = (struct pkt *) ((void *)tx_data + offsetof(struct usrp_transfer_frame, buf));
	printf("Address of tx_data = %p, p = %p\n", tx_data, p);

	printf("sizeof rp_transfer_frame = %d, sizeof pkt = %d\n", sizeof(struct usrp_transfer_frame), sizeof(struct pkt));

	for (i=0; i < packet_data_length; i++)
//		p->data[i] = random() >> 16;
		p->data[i] = i;

	tx_data->flags = 0;
	tx_data->len = 8 + packet_data_length * 2;

	printf("tx_data->len = %d\n", tx_data->len);

	seq_number = 1;
	tx_pkt_cnt = 0;

	while (1) {

		tx_pkt_cnt++;

#if 0
		if (tx_pkt_cnt  == 512) {
			printf(".");
			fflush(stdout);
		}
		if (tx_pkt_cnt  == 1024) {
			printf("'");
			fflush(stdout);
		}
		if (tx_pkt_cnt  == 1536) {
			printf(":");
			fflush(stdout);
			tx_pkt_cnt = 0;
		}
#endif

//		printf("tx flags = %X, len = %d\n", tx_data->flags, tx_data->len);
		p->seq_num = seq_number++;
		p->checksum = calc_checksum(p);
		cnt = write(fp, tx_data, 2048);
		if (cnt < 0)
			printf("Error returned from write: %d\n", cnt);

		bytes_transfered += tx_data->len;

		if (bytes_transfered > (100 * 1000000)) {
			gettimeofday(&finish_time, NULL);
			elapsed_seconds = finish_time.tv_sec - start_time.tv_sec;

			printf("TX data transfer rate = %f K Bps\n",
				(float) bytes_transfered / (float) elapsed_seconds / 1000);


			start_time = finish_time;
			bytes_transfered = 0;
                }
//		sleep(1);
	}
}


int main(int argc, char *argv[])
{
	pthread_t tx, rx;
	long int t;
	int fpga_config_flag ,decimation;
	struct usrp_e_ctl16 d;
	struct sched_param s = {
		.sched_priority = 1
	};

	if (argc < 4) {
		printf("%s t|w|rw decimation data_size\n", argv[0]);
		return -1;
	}

	decimation = atoi(argv[2]);
	packet_data_length = atoi(argv[3]);

	fp = open("/dev/usrp_e0", O_RDWR);
	printf("fp = %d\n", fp);

	fpga_config_flag = 0;
	if (strcmp(argv[1], "w") == 0)
		fpga_config_flag |= (1 << 15);
	else if (strcmp(argv[1], "r") == 0)
		fpga_config_flag |= (1 << 14);
	else if (strcmp(argv[1], "rw") == 0)
		fpga_config_flag |= ((1 << 15) | (1 << 14));

	fpga_config_flag |= decimation;

	d.offset = 14;
	d.count = 1;
	d.buf[0] = fpga_config_flag;
	ioctl(fp, USRP_E_WRITE_CTL16, &d);

	sleep(1); // in case the kernel threads need time to start. FIXME if so

	sched_setscheduler(0, SCHED_RR, &s);

	if (fpga_config_flag & (1 << 14)) {
		if (pthread_create(&rx, NULL, read_thread, (void *) t)) {
			printf("Failed to create rx thread\n");
			exit(-1);
		}
	}

	sleep(1);

	if (fpga_config_flag & (1 << 15)) {
		if (pthread_create(&tx, NULL, write_thread, (void *) t)) {
			printf("Failed to create tx thread\n");
			exit(-1);
		}
	}

	sleep(10000);

	printf("Done sleeping\n");
}
