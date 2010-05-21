#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include "usrp_e.h"

// max length #define PKT_DATA_LENGTH 1016
static int packet_data_length;

static int fp;
static u_int32_t crc_tab[256];

// CRC code from http://www.koders.com/c/fid699AFE0A656F0022C9D6B9D1743E697B69CE5815.aspx
// GPLv2

static u_int32_t chksum_crc32_gentab(void)
{
	unsigned long crc, poly;
	unsigned long i, j;

	poly = 0xEDB88320L;

	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 1) {
				crc = (crc >> 1) ^ poly;
			} else {
				crc >>= 1;
			}
		}
		crc_tab[i] = crc;
	}

	return 0;
}

static void *read_thread(void *threadid)
{
	int cnt;
	struct usrp_transfer_frame *rx_data;
	int rx_pkt_cnt;
	int i;
	unsigned long crc;
	unsigned int rx_crc;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;

	__u8 *p;
	__u32 *pi;

	printf("Greetings from the reading thread!\n");

	// IMPORTANT: must assume max length packet from fpga
	rx_data = malloc(2048);

	rx_pkt_cnt = 0;

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

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
		
		crc = 0xFFFFFFFF;
		for (i = 0; i < rx_data->len - 4; i++) {
			crc = ((crc >> 8) & 0x00FFFFFF) ^
				crc_tab[(crc ^ rx_data->buf[i]) & 0xFF];
		}

		p = &rx_data->buf[rx_data->len - 4];
		pi = (__u32 *) p;
		rx_crc = *pi;

#if 1
		printf("rx_data->len = %d\n", rx_data->len);
		printf("rx_data->flags = %d\n", rx_data->flags);
		for (i = 0; i < rx_data->len; i++)
			printf("idx = %d, data = %X\n", i, rx_data->buf[i]);
		printf("calc crc = %lX, rx crc = %X\n", crc, rx_crc); 
		fflush(stdout);
		break;
#endif

		if (rx_crc != (crc & 0xFFFFFFFF)) {
			printf("CRC Error, calc crc: %X, rx_crc: %X\n",
				(crc & 0xFFFFFFFF), rx_crc);
		}

		bytes_transfered += rx_data->len;

		if (bytes_transfered > (100 * 1000000)) {
			gettimeofday(&finish_time, NULL);
			elapsed_seconds = finish_time.tv_sec - start_time.tv_sec;

			printf("Bytes transfered = %ld, elapsed seconds = %ld\n", bytes_transfered, elapsed_seconds);
			printf("RX data transfer rate = %f K Samples/second\n",
				(float) bytes_transfered / (float) elapsed_seconds / 250);


			start_time = finish_time;
			bytes_transfered = 0;
		}
	}	
}
	
static void *write_thread(void *threadid)
{
	int seq_number, i, cnt, tx_pkt_cnt;
	int tx_len;
	unsigned long crc;
	struct usrp_transfer_frame *tx_data;
	unsigned long bytes_transfered, elapsed_seconds;
	struct timeval start_time, finish_time;

	printf("Greetings from the write thread!\n");

	tx_pkt_cnt = 0;
	tx_data = malloc(2048);

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

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

		tx_len = 2048 - sizeof(struct usrp_transfer_frame) - sizeof(int);
		tx_data->len = tx_len + sizeof(int);

		crc = 0xFFFFFFFF;
		for (i = 0; i < tx_len; i++) {
			tx_data->buf[i] = i & 0xFF;

			crc = ((crc >> 8) & 0x00FFFFFF) ^
				crc_tab[(crc ^ tx_data->buf[i]) & 0xFF];

		}
		*((int *) &tx_data[tx_len]) = crc;

		cnt = write(fp, tx_data, 2048);
		if (cnt < 0)
			printf("Error returned from write: %d\n", cnt);


		bytes_transfered += tx_data->len;

		if (bytes_transfered > (100 * 1000000)) {
			gettimeofday(&finish_time, NULL);
			elapsed_seconds = finish_time.tv_sec - start_time.tv_sec;

			printf("Bytes transfered = %d, elapsed seconds = %d\n", bytes_transfered, elapsed_seconds);
			printf("TX data transfer rate = %f K Samples/second\n",
				(float) bytes_transfered / (float) elapsed_seconds / 250);


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

	chksum_crc32_gentab();

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

	return 0;
}
