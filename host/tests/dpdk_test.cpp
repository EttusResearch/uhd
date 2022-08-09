//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * Benchmark program to check performance of 2 simultaneous links
 */


#include "common/mock_transport.hpp"
#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk/service_queue.hpp>
#include <uhdlib/transport/dpdk_io_service.hpp>
#include <uhdlib/transport/udp_dpdk_link.hpp>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <cstdbool>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace po = boost::program_options;

namespace {
constexpr unsigned int NUM_MBUFS       = 8192; /* Total number of mbufs in pool */
constexpr unsigned int MBUF_CACHE_SIZE = 384; /* Size of cpu-local mbuf cache */
constexpr unsigned int BURST_SIZE      = 64; /* Maximum burst size for RX */

constexpr unsigned int NUM_PORTS  = 2; /* Number of NIC ports */
constexpr unsigned int TX_CREDITS = 28; /* Number of TX credits */
constexpr unsigned int RX_CREDITS = 64; /* Number of RX credits */
constexpr unsigned int BENCH_SPP  = 700; /* "Samples" per packet */
} // namespace

struct dpdk_test_args
{
    unsigned int portid;
    std::string dst_ip;
    pthread_cond_t* cond;
    pthread_mutex_t mutex;
    bool started;
    int cpu;
};

struct dpdk_test_stats
{
    uint32_t last_seqno;
    uint32_t dropped_packets;
    uint32_t lasts[16];
    uint32_t drops[16];
    uint32_t last_ackno;
    uint32_t tx_seqno;
    uint64_t tx_xfer;
    uint32_t tx_no_bufs;
};


static void process_udp(int id, uint32_t* udp_data, struct dpdk_test_stats* stats)
{
    if (udp_data[0] != stats[id].last_seqno + 1) {
        stats[id].lasts[stats[id].dropped_packets & 0xf] = stats[id].last_seqno;
        stats[id].drops[stats[id].dropped_packets & 0xf] = udp_data[0];
        stats[id].dropped_packets++;
    }

    stats[id].last_seqno = udp_data[0];
    stats[id].last_ackno = udp_data[1];
}

static void send_udp(uhd::transport::mock_send_transport::sptr& stream,
    int id,
    bool fc_only,
    struct dpdk_test_stats* stats)
{
    uhd::transport::frame_buff::uptr mbuf = stream->get_data_buff(0);
    if (!mbuf) {
        printf("Could not get TX buffer!\n");
        stats[id].tx_no_bufs++;
        return;
    }
    uint32_t* tx_data;
    size_t buff_size;
    std::tie(tx_data, buff_size) = stream->buff_to_data(mbuf.get());
    tx_data[0]                   = fc_only ? stats[id].tx_seqno - 1 : stats[id].tx_seqno;
    tx_data[1]                   = stats[id].last_seqno;
    if (!fc_only) {
        memset(&tx_data[2], stats[id].last_seqno, 8 * BENCH_SPP);
        stats[id].tx_xfer += 8 * BENCH_SPP;
    }
    size_t num_bytes = 8 + (fc_only ? 0 : 8 * BENCH_SPP);
    stream->release_data_buff(mbuf, num_bytes / 4);

    if (!fc_only) {
        stats[id].tx_seqno++;
    }
}

static void bench(uhd::transport::mock_send_transport::sptr* tx_stream,
    uhd::transport::mock_recv_transport::sptr* rx_stream,
    uint32_t nb_ports,
    double timeout)
{
    uint64_t total_xfer[NUM_PORTS];
    uint32_t id;
    struct dpdk_test_stats* stats =
        (struct dpdk_test_stats*)malloc(sizeof(*stats) * nb_ports);
    for (id = 0; id < nb_ports; id++) {
        stats[id].tx_seqno        = 1;
        stats[id].tx_xfer         = 0;
        stats[id].last_ackno      = 0;
        stats[id].last_seqno      = 0;
        stats[id].dropped_packets = 0;
        total_xfer[id]            = 0;
    }
    sleep(1);
    struct timeval bench_start, bench_end;
    gettimeofday(&bench_start, NULL);
    /*
     * The test...
     */
    uint64_t total_received = 0;
    uint32_t consec_no_rx   = 0;
    while ((total_received / nb_ports) < 1000000) { //&& consec_no_rx < 10000) {
        for (id = 0; id < nb_ports; id++) {
            unsigned int nb_rx = 0;
            uhd::transport::frame_buff::uptr bufs[BURST_SIZE];
            for (; nb_rx < BURST_SIZE; nb_rx++) {
                bufs[nb_rx] = rx_stream[id]->get_data_buff(timeout);
                if (bufs[nb_rx] == nullptr) {
                    break;
                }
            }

            if (nb_rx <= 0) {
                if (timeout > 0.0) {
                    send_udp(tx_stream[id], id, true, stats);
                }
                consec_no_rx++;
                if (consec_no_rx >= 100000) {
                    // uint32_t skt_drops = stream[id]->get_drop_count();
                    // printf("TX seq %d, TX ack %d, RX seq %d, %d drops!\n",
                    // stats[id].tx_seqno, stats[id].last_ackno, stats[id].last_seqno,
                    // skt_drops);
                    consec_no_rx = 0;
                    break;
                }
                continue;
            } else {
                consec_no_rx = 0;
            }

            for (unsigned int buf = 0; buf < nb_rx; buf++) {
                total_xfer[id] += bufs[buf]->packet_size();
                uint32_t* data;
                size_t data_size;
                std::tie(data, data_size) = rx_stream[id]->buff_to_data(bufs[buf].get());
                process_udp(id, data, stats);
                rx_stream[id]->release_data_buff(std::move(bufs[buf]));
            }

            total_received += nb_rx;
        }

        for (id = 0; id < nb_ports; id++) {
            /* TX portion */
            uint32_t window_end = stats[id].last_ackno + TX_CREDITS;
            // uint32_t window_end = last_seqno[port] + TX_CREDITS;
            if (window_end <= stats[id].tx_seqno) {
                if (consec_no_rx == 9999) {
                    send_udp(tx_stream[id], id, true, stats);
                }
                // send_udp(tx[id], id, true);
                ;
            } else {
                for (unsigned int pktno = 0;
                     (pktno < BURST_SIZE) && (stats[id].tx_seqno < window_end);
                     pktno++) {
                    send_udp(tx_stream[id], id, false, stats);
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
        printf("Time taken = %ld us\n",
            (bench_end.tv_sec - bench_start.tv_sec) * 1000000
                + (bench_end.tv_usec - bench_start.tv_usec));
        double elapsed_time = (bench_end.tv_sec - bench_start.tv_sec) * 1000000
                              + (bench_end.tv_usec - bench_start.tv_usec);
        elapsed_time *= 1.0e-6;
        double elapsed_bytes = total_xfer[id];
        printf("RX Performance = %e Gbps\n", elapsed_bytes * 8.0 / 1.0e9 / elapsed_time);
        elapsed_bytes = stats[id].tx_xfer;
        printf("TX Performance = %e Gbps\n", elapsed_bytes * 8.0 / 1.0e9 / elapsed_time);
        // uint32_t skt_drops = stream[id]->get_drop_count();
        printf("Dropped %d packets\n", stats[id].dropped_packets);
        // printf("Socket reports dropped %d packets\n", skt_drops);
        for (unsigned int i = 0; i < 16; i++) {
            if (i >= stats[id].dropped_packets)
                break;
            printf("Last(%u), Recv(%u)\n", stats[id].lasts[i], stats[id].drops[i]);
        }
        // printf("%d missed/dropped packets\n", errors);
        printf("\n");
    }
    free(stats);
}

static inline void set_cpu(pthread_t t, int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    int status = pthread_setaffinity_np(t, sizeof(cpu_set_t), &cpuset);
    if (status) {
        perror("Could not set affinity");
    } else {
        printf("Set CPU to %d\n", cpu);
    }
}

std::string get_ipv4_addr(unsigned int port_id)
{
    auto ctx  = uhd::transport::dpdk::dpdk_ctx::get();
    auto port = ctx->get_port(port_id);
    auto ip   = port->get_ipv4();
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, addr_str, sizeof(addr_str));
    return std::string(addr_str);
}

void prepare_and_bench_polling(void)
{
    auto ctx = uhd::transport::dpdk::dpdk_ctx::get();

    uhd::transport::udp_dpdk_link::sptr eth_data[NUM_PORTS];
    uhd::transport::mock_send_transport::sptr tx_strm[NUM_PORTS];
    uhd::transport::mock_recv_transport::sptr rx_strm[NUM_PORTS];
    uhd::transport::link_params_t buff_args;
    buff_args.recv_frame_size = 8000;
    buff_args.send_frame_size = 8000;
    buff_args.num_send_frames = 32;
    buff_args.num_recv_frames = 32;
    auto dev_addr             = uhd::device_addr_t();
    eth_data[0]               = uhd::transport::udp_dpdk_link::make(
        0, get_ipv4_addr(1), "48888", "48888", buff_args);
    eth_data[1] = uhd::transport::udp_dpdk_link::make(
        1, get_ipv4_addr(0), "48888", "48888", buff_args);
    auto io_srv0 = ctx->get_io_service(0);
    io_srv0->attach_send_link(eth_data[0]);
    io_srv0->attach_recv_link(eth_data[0]);
    auto io_srv1 = ctx->get_io_service(1);
    io_srv1->attach_send_link(eth_data[1]);
    io_srv1->attach_recv_link(eth_data[1]);
    tx_strm[0] = std::make_shared<uhd::transport::mock_send_transport>(
        io_srv0, eth_data[0], eth_data[0], 0, 0, 32);
    rx_strm[0] = std::make_shared<uhd::transport::mock_recv_transport>(
        io_srv0, eth_data[0], eth_data[0], 1, 1, 32);
    tx_strm[1] = std::make_shared<uhd::transport::mock_send_transport>(
        io_srv1, eth_data[1], eth_data[1], 1, 1, 32);
    rx_strm[1] = std::make_shared<uhd::transport::mock_recv_transport>(
        io_srv1, eth_data[1], eth_data[1], 0, 0, 32);


    bench(tx_strm, rx_strm, NUM_PORTS, 0.0);
}

int main(int argc, char** argv)
{
    int status = 0;
    std::string args;
    std::string cpusets;
    po::options_description desc("Allowed options");
    desc.add_options()("help", "help message")(
        "args", po::value<std::string>(&args)->default_value(""), "UHD-DPDK args")(
        "polling-mode", "Use polling mode (single thread on own core)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    auto dpdk_args = uhd::device_addr_t(args);

    auto ctx = uhd::transport::dpdk::dpdk_ctx::get();
    ctx->init(args);

    if (vm.count("polling-mode")) {
        prepare_and_bench_polling();
    }
    return status;
}
