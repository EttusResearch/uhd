//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * Benchmark program to check performance of 2 simultaneous links
 */


#include <uhd-dpdk.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

#define NUM_MBUFS 4095
#define MBUF_CACHE_SIZE 315
#define BURST_SIZE 64

#define NUM_PORTS 2
#define TX_CREDITS 28
#define RX_CREDITS 64
#define BENCH_SPP 700
#define BENCH_IFG 220


static uint32_t last_seqno[NUM_PORTS];
static uint32_t dropped_packets[NUM_PORTS];
static uint32_t lasts[NUM_PORTS][16], drops[NUM_PORTS][16];
static uint32_t last_ackno[NUM_PORTS];
static uint32_t tx_seqno[NUM_PORTS];
static uint64_t tx_xfer[NUM_PORTS];

static void process_udp(int id, uint32_t *udp_data)
{
	if (udp_data[0] != last_seqno[id] + 1) {
		lasts[id][dropped_packets[id] & 0xf] = last_seqno[id];
		drops[id][dropped_packets[id] & 0xf] = udp_data[0];
		dropped_packets[id]++;
	}

	last_seqno[id] = udp_data[0];
	last_ackno[id] = udp_data[1];
}

static void send_ctrl(struct uhd_dpdk_socket *sock, uint32_t run)
{
	struct rte_mbuf *mbuf = NULL;
	uhd_dpdk_request_tx_bufs(sock, &mbuf, 1);
	if (unlikely(mbuf == NULL))
		return;
	uint32_t *tx_data = uhd_dpdk_buf_to_data(sock, mbuf);
	tx_data[0] = (BENCH_SPP << 16) | (TX_CREDITS << 4) | run; // spp, tx_credits, run
	tx_data[1] = (RX_CREDITS << 16) | (BENCH_IFG); // credits, ifg
	mbuf->pkt_len = 8;
	mbuf->data_len = 8;
	uhd_dpdk_send(sock, &mbuf, 1);
}

static void send_udp(struct uhd_dpdk_socket *sock, int id, bool fc_only)
{
	struct rte_mbuf *mbuf = NULL;
	uhd_dpdk_request_tx_bufs(sock, &mbuf, 1);
	if (unlikely(mbuf == NULL))
		return;
	uint32_t *tx_data = uhd_dpdk_buf_to_data(sock, mbuf);
	tx_data[0] = fc_only ? tx_seqno[id] - 1 : tx_seqno[id];
	tx_data[1] = last_seqno[id];
	if (!fc_only) {
		memset(&tx_data[2], last_seqno[id], 8*BENCH_SPP);
		tx_xfer[id] += 8*BENCH_SPP;
	}
	mbuf->pkt_len = 8 + (fc_only ? 0 : 8*BENCH_SPP);
	mbuf->data_len = 8 + (fc_only ? 0 : 8*BENCH_SPP);

	uhd_dpdk_send(sock, &mbuf, 1);

	if (!fc_only) {
		tx_seqno[id]++;
	}
}

static void bench(struct uhd_dpdk_socket **tx, struct uhd_dpdk_socket **rx, struct uhd_dpdk_socket **ctrl, uint32_t nb_ports)
{
	uint64_t total_xfer[NUM_PORTS];
	uint32_t id;
	for (id = 0; id < nb_ports; id++) {
		tx_seqno[id] = 1;
		tx_xfer[id] = 0;
		last_ackno[id] = 0;
		last_seqno[id] = 0;
		dropped_packets[id] = 0;
		total_xfer[id] = 0;
	}
	sleep(1);
	struct timeval bench_start, bench_end;
	gettimeofday(&bench_start, NULL);
	for (id = 0; id < nb_ports; id++) {
		send_ctrl(ctrl[id], 0);
		for (int pktno = 0; (pktno < TX_CREDITS*3/4); pktno++) {
			send_udp(tx[id], id, false);
		}
	}
	for (id = 0; id < nb_ports; id++) {
		send_ctrl(ctrl[id], 1);
	}
	/*
	 * The test...
	 */
	uint64_t total_received = 0;
	uint32_t consec_no_rx = 0;
	while (total_received < 1000000 ) { //&& consec_no_rx < 10000) {
		for (id = 0; id < nb_ports; id++) {

			/* Get burst of RX packets, from first port of pair. */
			struct rte_mbuf *bufs[BURST_SIZE];
			const int64_t nb_rx = uhd_dpdk_recv(rx[id], bufs, BURST_SIZE);

			if (unlikely(nb_rx <= 0)) {
				consec_no_rx++;
				if (consec_no_rx >= 100000) {
					uint32_t skt_drops = 0;
					uhd_dpdk_get_drop_count(rx[id], &skt_drops);
					printf("TX seq %d, TX ack %d, RX seq %d, %d, drops!\n", tx_seqno[id], last_ackno[id], last_seqno[id], skt_drops);
					consec_no_rx = 0;
					break;
				}
				continue;
			} else {
				consec_no_rx = 0;
			}

			for (int buf = 0; buf < nb_rx; buf++) {
				total_xfer[id] += bufs[buf]->pkt_len;
				uint64_t ol_flags = bufs[buf]->ol_flags;
				uint32_t *data = (uint32_t *) uhd_dpdk_buf_to_data(rx[id], bufs[buf]);
				if (ol_flags == PKT_RX_IP_CKSUM_BAD) { /* FIXME: Deprecated/changed in later release */
					printf("Buf %d: Bad IP cksum\n", buf);
				} else {
					process_udp(id, data);
				}
			}

			/* Free buffers. */
			for (int buf = 0; buf < nb_rx; buf++)
				uhd_dpdk_free_buf(bufs[buf]);
			total_received += nb_rx;
		}

		for (id = 0; id < nb_ports; id++) {
			/* TX portion */
			uint32_t window_end = last_ackno[id] + TX_CREDITS;
			//uint32_t window_end = last_seqno[port] + TX_CREDITS;
			if (window_end <= tx_seqno[id]) {
				if (consec_no_rx == 9999) {
					send_udp(tx[id], id, true);
				}
				//send_udp(tx[id], id, true);
				;
			} else {
				for (int pktno = 0; (pktno < BURST_SIZE) && (tx_seqno[id] < window_end); pktno++) {
					send_udp(tx[id], id, false);
				}
			}
		}
	}
	for (id = 0; id < nb_ports; id++) {
		send_ctrl(ctrl[id], 0);
	}
	gettimeofday(&bench_end, NULL);
	printf("Benchmark complete\n\n");

	for (id = 0; id < nb_ports; id++) {
		printf("\n");
		printf("Bytes received = %ld\n", total_xfer[id]);
		printf("Bytes sent = %ld\n", tx_xfer[id]);
		printf("Time taken = %ld us\n", (bench_end.tv_sec - bench_start.tv_sec)*1000000 + (bench_end.tv_usec - bench_start.tv_usec));
		double elapsed_time = (bench_end.tv_sec - bench_start.tv_sec)*1000000 + (bench_end.tv_usec - bench_start.tv_usec);
		elapsed_time *= 1.0e-6;
		double elapsed_bytes = total_xfer[id];
		printf("RX Performance = %e Gbps\n", elapsed_bytes*8.0/1.0e9/elapsed_time);
		elapsed_bytes = tx_xfer[id];
		printf("TX Performance = %e Gbps\n", elapsed_bytes*8.0/1.0e9/elapsed_time);
		uint32_t skt_drops = 0;
		uhd_dpdk_get_drop_count(rx[id], &skt_drops);
		printf("Dropped %d packets\n", dropped_packets[id]);
		printf("Socket reports dropped %d packets\n", skt_drops);
		for (unsigned int i = 0; i < 16; i++) {
			if (i >= dropped_packets[id])
				break;
			printf("Last(%u), Recv(%u)\n", lasts[id][i], drops[id][i]);
		}
		//printf("%d missed/dropped packets\n", errors);
		printf("\n");
	}

}

int main(int argc, char **argv)
{
	int port_thread_mapping[2] = {1, 1};
	int status = uhd_dpdk_init(argc, argv, 2, &port_thread_mapping[0], NUM_MBUFS, MBUF_CACHE_SIZE);
	if (status) {
		printf("%d: Something wrong?\n", status);
		return status;
	}

	uint32_t eth_ip = htonl(0xc0a80008);
	uint32_t eth_mask = htonl(0xffffff00);
	status = uhd_dpdk_set_ipv4_addr(0, eth_ip, eth_mask);
	if (status) {
		printf("Error while setting IP0: %d\n", status);
		return status;
	}
	status = uhd_dpdk_set_ipv4_addr(1, eth_ip, eth_mask);
	if (status) {
		printf("Error while setting IP1: %d\n", status);
		return status;
	}

	struct uhd_dpdk_socket *eth_rx[2];
	struct uhd_dpdk_socket *eth_tx[2];
	struct uhd_dpdk_socket *eth_ctrl[2];
	struct uhd_dpdk_sockarg_udp sockarg = {
		.is_tx = false,
		.local_port = htons(0xBEE7),
		.remote_port = htons(0xBEE7),
		.dst_addr = htonl(0xc0a80004)
	};
	eth_rx[0] = uhd_dpdk_sock_open(0, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_rx[0]) {
		printf("!eth0_rx\n");
		return -ENODEV;
	}
	eth_rx[1] = uhd_dpdk_sock_open(1, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_rx[1]) {
		printf("!eth1_rx\n");
		return -ENODEV;
	}

	sockarg.is_tx = true;
	eth_tx[0] = uhd_dpdk_sock_open(0, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_tx[0]) {
		printf("!eth0_tx\n");
		return -ENODEV;
	}
	eth_tx[1] = uhd_dpdk_sock_open(1, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_tx[1]) {
		printf("!eth1_tx\n");
		return -ENODEV;
	}

	sockarg.local_port = htons(0xB4D);
	sockarg.remote_port = htons(0xB4D);
	eth_ctrl[0] = uhd_dpdk_sock_open(0, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_ctrl[0]) {
		printf("!eth0_ctrl\n");
		return -ENODEV;
	}
	eth_ctrl[1] = uhd_dpdk_sock_open(1, UHD_DPDK_SOCK_UDP, &sockarg);
	if (!eth_ctrl[1]) {
		printf("!eth1_ctrl\n");
		return -ENODEV;
	}

	bench(eth_tx, eth_rx, eth_ctrl, 2);

	status = uhd_dpdk_sock_close(eth_rx[0]);
	if (status) {
		printf("Bad close RX0 %d\n", status);
		return status;
	}
	status = uhd_dpdk_sock_close(eth_rx[1]);
	if (status) {
		printf("Bad close RX1 %d\n", status);
		return status;
	}
	status = uhd_dpdk_sock_close(eth_tx[0]);
	if (status) {
		printf("Bad close TX0 %d\n", status);
		return status;
	}
	status = uhd_dpdk_sock_close(eth_tx[1]);
	if (status) {
		printf("Bad close TX1 %d\n", status);
		return status;
	}
	status = uhd_dpdk_sock_close(eth_ctrl[0]);
	if (status) {
		printf("Bad close Ctrl0 %d\n", status);
		return status;
	}
	status = uhd_dpdk_sock_close(eth_ctrl[1]);
	if (status) {
		printf("Bad close Ctrl1 %d\n", status);
		return status;
	}
	return status;
}
