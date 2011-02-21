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
#include <poll.h>
#include <sys/mman.h>
#include "linux/usrp_e.h"

// max length #define PKT_DATA_LENGTH 1016
static int packet_data_length;

struct ring_buffer_info (*rxi)[];
struct ring_buffer_info (*txi)[];
__u8 *rx_buf;
__u8 *tx_buf;
static struct usrp_e_ring_buffer_size_t rb_size;

static int fp;
static u_int32_t crc_tab[256];

// CRC code from http://www.koders.com/c/fid699AFE0A656F0022C9D6B9D1743E697B69CE5815.aspx
// GPLv2

static u_int32_t chksum_crc32_gentab(void)
{
	unsigned long crc, poly;
	unsigned long i, j;

	poly = 0x04C11DB7L;

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
//		printf("crc_tab[%d] = %X\n", i , crc);
	}

	return 0;
}

struct timeval delta_time(struct timeval f, struct timeval s)
{
	struct timeval d;

	if (f.tv_usec > s.tv_usec) {
		d.tv_usec = f.tv_usec - s.tv_usec;
		d.tv_sec = f.tv_sec - s.tv_sec;
	} else {
		d.tv_usec = f.tv_usec - s.tv_usec + 1e6;
		d.tv_sec = f.tv_sec - s.tv_sec - 1;
	}

	return d;
}


static void *read_thread(void *threadid)
{
	unsigned int cnt;
	int rx_pkt_cnt, rb_read;
	unsigned int i;
	unsigned long crc, ck_sum;
	unsigned int rx_crc, pkt_len, pkt_seq;
	unsigned long bytes_transfered;
	struct timeval start_time;
	unsigned int prev_seq = 0;
	int first = 1;
	long tid;
	__u8 *p;


	tid = (long)threadid;
	printf("Greetings from the reading thread(%ld)!\n", tid);

	// IMPORTANT: must assume max length packet from fpga
	
	rx_pkt_cnt = 0;
	rb_read = 0;

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

	while (1) {
		
		while (!((*rxi)[rb_read].flags & RB_USER)) {
			struct pollfd pfd;
			pfd.fd = fp;
			pfd.events = POLLIN;
			poll(&pfd, 1, -1);
		}
		(*rxi)[rb_read].flags = RB_USER_PROCESS;

		rx_pkt_cnt++;
		cnt = (*rxi)[rb_read].len;
		p = rx_buf + (rb_read * 2048);

		rx_crc = *(int *) &p[cnt-4];
		crc = 0xFFFFFFFF;
		ck_sum = 0;

		pkt_len = *(unsigned int *) &p[0];
		pkt_seq = *(unsigned int *) &p[4];

//		printf("Pkt len = %X, pkt seq = %X, driver len = %X\n", pkt_len, pkt_seq, cnt);

		if (pkt_len != (cnt - 4))
			printf("Packet length check fail, driver len = %ud, content = %ud\n",
					cnt, pkt_len);

		if (!first && (pkt_seq != (prev_seq + 1)))
			printf("Sequence number check fail, pkt_seq = %ud, prev_seq = %ud\n",
					pkt_seq, prev_seq);
		first = 0;
		prev_seq = pkt_seq;

		for (i = 0; i < cnt-4; i++) {
			ck_sum += p[i];

			crc = ((crc >> 8) & 0x00FFFFFF) ^
				crc_tab[(crc ^ p[i]) & 0xFF];
//printf("idx = %d, data = %X, crc = %X, ck_sum = %X\n", i, p[i], crc, ck_sum);
//			crc = ((crc >> 8) & 0x00FFFFFF) ^
//				crc_tab[(crc ^ p[i+1]) & 0xFF];
//printf("idx = %d, data = %X, crc = %X\n", i, p[i+1],crc);
		}

		(*rxi)[rb_read].flags = RB_KERNEL;
		write(fp, NULL, 1);

		if (rx_crc != ck_sum)
			printf("Ck_sum eror, calc ck_sum = %lX, rx ck_sum = %X\n",
					ck_sum, rx_crc);

#if 0
		if (rx_crc != (crc & 0xFFFFFFFF)) {
			printf("CRC Error, calc crc: %X, rx_crc: %X\n",
				(crc & 0xFFFFFFFF), rx_crc);
		}
#endif

		rb_read++;
		if (rb_read == rb_size.num_rx_frames)
			rb_read = 0;

		bytes_transfered += cnt;

		if (bytes_transfered > (100 * 1000000)) {
			struct timeval finish_time, d_time;
			float elapsed_seconds;

			gettimeofday(&finish_time, NULL);

			printf("sec = %ld, usec = %ld\n", finish_time.tv_sec, finish_time.tv_usec);

			d_time = delta_time(finish_time, start_time);

			elapsed_seconds = (float)d_time.tv_sec + ((float)d_time.tv_usec * 1e-6f);

			printf("Bytes transfered = %ld, elapsed seconds = %f\n", bytes_transfered, elapsed_seconds);
			printf("RX data transfer rate = %f K Samples/second\n",
				(float) bytes_transfered / (float) elapsed_seconds / 4000);


			start_time = finish_time;
			bytes_transfered = 0;
		}
	}	
	return NULL;
}
	
static void *write_thread(void *threadid)
{
	int i, tx_pkt_cnt, rb_write;
	int tx_len;
	unsigned long crc;
	unsigned long bytes_transfered;
	struct timeval start_time;
	unsigned int pkt_seq = 0;
	long tid;
	__u8 *p;

	tid = (long)threadid;
	printf("Greetings from the write thread(%ld)!\n", tid);

	rb_write = 0;
	tx_pkt_cnt = 0;

	bytes_transfered = 0;
	gettimeofday(&start_time, NULL);

	while (1) {

		tx_pkt_cnt++;
		p = tx_buf + (rb_write * 2048);

//		printf("p = %p\n", p);

		if (packet_data_length > 0)
			tx_len = packet_data_length;
		else
			tx_len = (random() & 0x1ff) + (2044 - 512);

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

//		printf("Checking for space at rb entry = %d\n", rb_write);
		while (!((*txi)[rb_write].flags & RB_KERNEL)) {
			struct pollfd pfd;
			pfd.fd = fp;
			pfd.events = POLLOUT;
			poll(&pfd, 1, -1);
		}
//		printf("Got space\n");

		for (i=8; i < tx_len-4; i++) {
			p[i] = i & 0xFF;
		}

		*(unsigned int *) &p[0] = tx_len-4;
		*(unsigned int *) &p[4] = pkt_seq;

		pkt_seq++;

		crc = 0xFFFFFFFF;
		for (i = 0; i < tx_len-4; i++) {
//			printf("%X ", p[i]);
			crc = ((crc >> 8) & 0x00FFFFFF) ^
				crc_tab[(crc ^ p[i]) & 0xFF];

		}
		*(unsigned int *) &p[tx_len-4] = crc;
//		printf("\n crc = %lX\n", crc);

		(*txi)[rb_write].len = tx_len;
		(*txi)[rb_write].flags = RB_USER;

		rb_write++;
		if (rb_write == rb_size.num_tx_frames)
			rb_write = 0;

		bytes_transfered += tx_len;

		if (bytes_transfered > (100 * 1000000)) {
			struct timeval finish_time, d_time;
			float elapsed_seconds;

			gettimeofday(&finish_time, NULL);

			d_time = delta_time(finish_time, start_time);

			elapsed_seconds = (float)d_time.tv_sec - ((float)d_time.tv_usec * 1e-6f);

			printf("Bytes transfered = %ld, elapsed seconds = %f\n", bytes_transfered, elapsed_seconds);
			printf("TX data transfer rate = %f K Samples/second\n",
				(float) bytes_transfered / (float) elapsed_seconds / 4000);


			start_time = finish_time;
			bytes_transfered = 0;
		}

//		sleep(1);
	}
	return NULL;
}


int main(int argc, char *argv[])
{
	pthread_t tx, rx;
	long int t=0;
	int fpga_config_flag ,decimation;
	int ret, map_size, page_size;
	void *rb;

	struct usrp_e_ctl16 d;
	struct sched_param s = {
		.sched_priority = 1
	};

	if (argc < 4) {
		printf("%s r|w|rw decimation data_size\n", argv[0]);
		return -1;
	}

	chksum_crc32_gentab();

	decimation = atoi(argv[2]);
	packet_data_length = atoi(argv[3]);

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

	printf("rb = %p\n", rb);

	rxi = rb;
	rx_buf = rb + (rb_size.num_pages_rx_flags * page_size);
	txi = rb + (rb_size.num_pages_rx_flags * page_size) +
		(rb_size.num_rx_frames * page_size >> 1);
	tx_buf = rb + (rb_size.num_pages_rx_flags * page_size) +
		(rb_size.num_rx_frames * page_size >> 1) +
		(rb_size.num_pages_tx_flags * page_size);

	fpga_config_flag = (1<<8);
	if (strcmp(argv[1], "w") == 0)
		fpga_config_flag |= (1 << 11);
	else if (strcmp(argv[1], "r") == 0)
		fpga_config_flag |= (1 << 10);
	else if (strcmp(argv[1], "rw") == 0)
		fpga_config_flag |= ((1 << 10) | (1 << 11));

	fpga_config_flag |= decimation;

	d.offset = 14;
	d.count = 1;
	d.buf[0] = fpga_config_flag;
	ioctl(fp, USRP_E_WRITE_CTL16, &d);

	sleep(1); // in case the kernel threads need time to start. FIXME if so

	sched_setscheduler(0, SCHED_RR, &s);

	if (fpga_config_flag & (1 << 10)) {
		if (pthread_create(&rx, NULL, read_thread, (void *) t)) {
			printf("Failed to create rx thread\n");
			exit(-1);
		}
	}

	sleep(1);

	if (fpga_config_flag & (1 << 11)) {
		if (pthread_create(&tx, NULL, write_thread, (void *) t)) {
			printf("Failed to create tx thread\n");
			exit(-1);
		}
	}

	sleep(10000);

	printf("Done sleeping\n");

	return 0;
}
