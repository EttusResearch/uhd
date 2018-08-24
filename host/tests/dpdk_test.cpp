//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * Benchmark program to check performance of 2 simultaneous links
 */


#include <cstdio>
#include <cstdbool>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sched.h>
#include <sys/syscall.h>
#include "../transport/dpdk_zero_copy.hpp"
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

namespace {
    constexpr unsigned int NUM_MBUFS       = 4095; /* Total number of mbufs in pool */
    constexpr unsigned int MBUF_CACHE_SIZE = 315; /* Size of cpu-local mbuf cache */
    constexpr unsigned int BURST_SIZE      = 64; /* Maximum burst size for RX */

    constexpr unsigned int NUM_PORTS       = 2; /* Number of NIC ports */
    constexpr unsigned int TX_CREDITS      = 28; /* Number of TX credits */
    constexpr unsigned int RX_CREDITS      = 64; /* Number of RX credits */
    constexpr unsigned int BENCH_SPP       = 700; /* "Samples" per packet */
}

struct dpdk_test_args {
    unsigned int portid;
    std::string src_port;
    std::string dst_ip;
    std::string dst_port;
};

struct dpdk_test_stats {
    uint32_t last_seqno;
    uint32_t dropped_packets;
    uint32_t lasts[16];
    uint32_t drops[16];
    uint32_t last_ackno;
    uint32_t tx_seqno;
    uint64_t tx_xfer;
};

static void process_udp(int id, uint32_t *udp_data, struct dpdk_test_stats *stats)
{
    if (udp_data[0] != stats[id].last_seqno + 1) {
        stats[id].lasts[stats[id].dropped_packets & 0xf] = stats[id].last_seqno;
        stats[id].drops[stats[id].dropped_packets & 0xf] = udp_data[0];
        stats[id].dropped_packets++;
    }

    stats[id].last_seqno = udp_data[0];
    stats[id].last_ackno = udp_data[1];
}

static void send_udp(uhd::transport::dpdk_zero_copy::sptr &stream, int id,
                     bool fc_only, struct dpdk_test_stats *stats)
{
    uhd::transport::managed_send_buffer::sptr mbuf = stream->get_send_buff(0);
    if (mbuf.get() == nullptr) {
        printf("Could not get TX buffer!\n");
        return;
    }
    auto *tx_data = mbuf->cast<uint32_t *>();
    tx_data[0] = fc_only ? stats[id].tx_seqno - 1 : stats[id].tx_seqno;
    tx_data[1] = stats[id].last_seqno;
    if (!fc_only) {
        memset(&tx_data[2], stats[id].last_seqno, 8*BENCH_SPP);
        stats[id].tx_xfer += 8*BENCH_SPP;
    }
    size_t num_bytes = 8 + (fc_only ? 0 : 8*BENCH_SPP);
    mbuf->commit(num_bytes);
    mbuf.reset();

    if (!fc_only) {
        stats[id].tx_seqno++;
    }
}

static void bench(uhd::transport::dpdk_zero_copy::sptr *stream, uint32_t nb_ports, double timeout)
{
    uint64_t total_xfer[NUM_PORTS];
    uint32_t id;
    struct dpdk_test_stats *stats = (struct dpdk_test_stats *) malloc(sizeof(*stats)*nb_ports);
    for (id = 0; id < nb_ports; id++) {
        stats[id].tx_seqno = 1;
        stats[id].tx_xfer = 0;
        stats[id].last_ackno = 0;
        stats[id].last_seqno = 0;
        stats[id].dropped_packets = 0;
        total_xfer[id] = 0;
    }
    sleep(1);
    struct timeval bench_start, bench_end;
    gettimeofday(&bench_start, NULL);
    /*
     * The test...
     */
    uint64_t total_received = 0;
    uint32_t consec_no_rx = 0;
    while (total_received < 1000000 ) { //&& consec_no_rx < 10000) {
        for (id = 0; id < nb_ports; id++) {
            unsigned int nb_rx = 0;
            uhd::transport::managed_recv_buffer::sptr bufs[BURST_SIZE];
            for (; nb_rx < BURST_SIZE; nb_rx++) {
                bufs[nb_rx] = stream[id]->get_recv_buff(timeout);
                if (bufs[nb_rx].get() == nullptr) {
                    bufs[nb_rx].reset();
                    break;
                }
            }

            if (nb_rx <= 0) {
                consec_no_rx++;
                if (consec_no_rx >= 100000) {
                    uint32_t skt_drops = stream[id]->get_drop_count();
                    //printf("TX seq %d, TX ack %d, RX seq %d, %d drops!\n", stats[id].tx_seqno, stats[id].last_ackno, stats[id].last_seqno, skt_drops);
                    consec_no_rx = 0;
                    break;
                }
                continue;
            } else {
                consec_no_rx = 0;
            }

            for (unsigned int buf = 0; buf < nb_rx; buf++) {
                total_xfer[id] += bufs[buf]->size();
                auto data = bufs[buf]->cast<uint32_t *>();
                process_udp(id, data, stats);
            }

            total_received += nb_rx;
        }

        for (id = 0; id < nb_ports; id++) {
            /* TX portion */
            uint32_t window_end = stats[id].last_ackno + TX_CREDITS;
            //uint32_t window_end = last_seqno[port] + TX_CREDITS;
            if (window_end <= stats[id].tx_seqno) {
                if (consec_no_rx == 9999) {
                    send_udp(stream[id], id, true, stats);
                }
                //send_udp(tx[id], id, true);
                ;
            } else {
                for (unsigned int pktno = 0; (pktno < BURST_SIZE) && (stats[id].tx_seqno < window_end); pktno++) {
                    send_udp(stream[id], id, false, stats);
                }
            }
        }
    }
    gettimeofday(&bench_end, NULL);
    printf("Benchmark complete\n\n");

    for (id = 0; id < nb_ports; id++) {
        printf("\n");
        printf("Bytes received = %ld\n", total_xfer[id]);
        printf("Bytes sent = %ld\n", stats[id].tx_xfer);
        printf("Time taken = %ld us\n", (bench_end.tv_sec - bench_start.tv_sec)*1000000 + (bench_end.tv_usec - bench_start.tv_usec));
        double elapsed_time = (bench_end.tv_sec - bench_start.tv_sec)*1000000 + (bench_end.tv_usec - bench_start.tv_usec);
        elapsed_time *= 1.0e-6;
        double elapsed_bytes = total_xfer[id];
        printf("RX Performance = %e Gbps\n", elapsed_bytes*8.0/1.0e9/elapsed_time);
        elapsed_bytes = stats[id].tx_xfer;
        printf("TX Performance = %e Gbps\n", elapsed_bytes*8.0/1.0e9/elapsed_time);
        uint32_t skt_drops = stream[id]->get_drop_count();
        printf("Dropped %d packets\n", stats[id].dropped_packets);
        printf("Socket reports dropped %d packets\n", skt_drops);
        for (unsigned int i = 0; i < 16; i++) {
            if (i >= stats[id].dropped_packets)
                break;
            printf("Last(%u), Recv(%u)\n", stats[id].lasts[i], stats[id].drops[i]);
        }
        //printf("%d missed/dropped packets\n", errors);
        printf("\n");
    }
    free(stats);

}

void *prepare_and_bench_blocking(void *arg)
{
    struct dpdk_test_args *args = (struct dpdk_test_args *) arg;
    pthread_t t = pthread_self();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    /* FIXME: Make an argument to the test */
    CPU_SET(4+args->portid, &cpuset);
    int status = pthread_setaffinity_np(t, sizeof(cpu_set_t), &cpuset);
    if (status) {
        perror("Could not set affinity");
    }
    auto &ctx = uhd::transport::uhd_dpdk_ctx::get();
    uhd::transport::dpdk_zero_copy::sptr eth_data[1];
    uhd::transport::zero_copy_xport_params buff_args;
    buff_args.recv_frame_size = 8000;
    buff_args.send_frame_size = 8000;
    buff_args.num_send_frames = 8;
    buff_args.num_recv_frames = 8;
    auto dev_addr = uhd::device_addr_t();
    eth_data[0] = uhd::transport::dpdk_zero_copy::make(
        ctx,
        args->portid,
        args->dst_ip,
        args->src_port,
        args->dst_port,
        buff_args,
        dev_addr
    );

    bench(eth_data, 1, 0.1);
    return 0;
}

void prepare_and_bench_polling(void)
{
    auto &ctx = uhd::transport::uhd_dpdk_ctx::get();
    struct dpdk_test_args bench_args[2] = {
        {
            .portid = 0,
            .src_port = "0xBEE7",
            .dst_ip = "192.168.0.4",
            .dst_port = "0xBEE7",
        },
        {
            .portid = 1,
            .src_port = "0xBEE7",
            .dst_ip = "192.168.0.3",
            .dst_port = "0xBEE7",
        }
    };
    uhd::transport::dpdk_zero_copy::sptr eth_data[NUM_PORTS];
    uhd::transport::zero_copy_xport_params buff_args;
    buff_args.recv_frame_size = 8000;
    buff_args.send_frame_size = 8000;
    buff_args.num_send_frames = 8;
    buff_args.num_recv_frames = 8;
    auto dev_addr = uhd::device_addr_t();
    for (unsigned int i = 0; i < NUM_PORTS; i++) {
        eth_data[i] = uhd::transport::dpdk_zero_copy::make(
            ctx,
            bench_args[i].portid,
            bench_args[i].dst_ip,
            bench_args[i].src_port,
            bench_args[i].dst_port,
            buff_args,
            dev_addr
        );
    }

    bench(eth_data, NUM_PORTS, 0.0);
}

int main(int argc, char **argv)
{
    int retval;
    std::string args, core_map;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "UHD-DPDK args")
        ("polling-mode", "Use polling mode (single thread on own core)")
        ("core-map", po::value<std::string>(&core_map)->default_value(""), "which core(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
       std::cout << desc << std::endl;
       return 0;
    }

    int port_thread_mapping[2] = {1, 1};

    auto &ctx = uhd::transport::uhd_dpdk_ctx::get();
    auto dpdk_args = uhd::dict<std::string, std::string>();
    //dpdk_args.set("key", "val");
    ctx.init(dpdk_args, 2, &port_thread_mapping[0], NUM_MBUFS, MBUF_CACHE_SIZE, 9000);

    uint32_t eth_ip = htonl(0xc0a80003);
    uint32_t eth_mask = htonl(0xffffff00);
    int status = ctx.set_ipv4_addr(0, eth_ip, eth_mask);
    if (status) {
        printf("Error while setting IP0: %d\n", status);
        return status;
    }
    eth_ip = htonl(0xc0a80004);
    status = ctx.set_ipv4_addr(1, eth_ip, eth_mask);
    if (status) {
        printf("Error while setting IP1: %d\n", status);
        return status;
    }

    if (vm.count("polling-mode")) {
        prepare_and_bench_polling();
    } else {
        struct dpdk_test_args bench_args[2] = {
            {
                .portid = 0,
                .src_port = "0xBEE7",
                .dst_ip = "192.168.0.4",
                .dst_port = "0xBEE7",
            },
            {
                .portid = 1,
                .src_port = "0xBEE7",
                .dst_ip = "192.168.0.3",
                .dst_port = "0xBEE7",
            }
        };
        pthread_t threads[2];
        pthread_create(&threads[0], NULL, prepare_and_bench_blocking, &bench_args[0]);
        pthread_create(&threads[1], NULL, prepare_and_bench_blocking, &bench_args[1]);

        status = pthread_join(threads[0], (void **) &retval);
        if (status) {
            perror("Error while joining thread");
            return status;
        }
        status = pthread_join(threads[1], (void **) &retval);
        if (status) {
            perror("Error while joining thread");
            return status;
        }
    }

    return status;
}
