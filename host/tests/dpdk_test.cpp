//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * Benchmark program to check performance of 2 simultaneous links
 */


#include <uhdlib/transport/dpdk_zero_copy.hpp>
#include <uhdlib/transport/uhd-dpdk.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <cstdbool>
#include <cstdio>
#include <cstring>
#include <iostream>

static const boost::regex colons(":");

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
    pthread_cond_t *cond;
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

static void send_udp(uhd::transport::dpdk_zero_copy::sptr& stream,
    int id,
    bool fc_only,
    struct dpdk_test_stats *stats)
{
    uhd::transport::managed_send_buffer::sptr mbuf = stream->get_send_buff(0);
    if (mbuf.get() == nullptr) {
        printf("Could not get TX buffer!\n");
        return;
    }
    auto *tx_data = mbuf->cast<uint32_t *>();
    tx_data[0]    = fc_only ? stats[id].tx_seqno - 1 : stats[id].tx_seqno;
    tx_data[1]    = stats[id].last_seqno;
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

static void bench(
    uhd::transport::dpdk_zero_copy::sptr *stream, uint32_t nb_ports, double timeout)
{
    uint64_t total_xfer[NUM_PORTS];
    uint32_t id;
    struct dpdk_test_stats *stats = (struct dpdk_test_stats *) malloc(sizeof(*stats)*nb_ports);
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
    while ((total_received / nb_ports) < 10000000) { //&& consec_no_rx < 10000) {
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
                if (timeout > 0.0) {
                    send_udp(stream[id], id, true, stats);
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
                total_xfer[id] += bufs[buf]->size();
                auto data = bufs[buf]->cast<uint32_t *>();
                process_udp(id, data, stats);
            }

            total_received += nb_rx;
        }

        for (id = 0; id < nb_ports; id++) {
            /* TX portion */
            uint32_t window_end = stats[id].last_ackno + TX_CREDITS;
            // uint32_t window_end = last_seqno[port] + TX_CREDITS;
            if (window_end <= stats[id].tx_seqno) {
                if (consec_no_rx == 9999) {
                    send_udp(stream[id], id, true, stats);
                }
                // send_udp(tx[id], id, true);
                ;
            } else {
                for (unsigned int pktno = 0;
                     (pktno < BURST_SIZE) && (stats[id].tx_seqno < window_end);
                     pktno++) {
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
        printf("Time taken = %ld us\n",
            (bench_end.tv_sec - bench_start.tv_sec)*1000000
                + (bench_end.tv_usec - bench_start.tv_usec));
        double elapsed_time = (bench_end.tv_sec - bench_start.tv_sec)*1000000
            + (bench_end.tv_usec - bench_start.tv_usec);
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
    struct in_addr ipv4_addr;
    int status = uhd_dpdk_get_ipv4_addr(port_id, &ipv4_addr.s_addr, NULL);
    UHD_ASSERT_THROW(status == 0);
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipv4_addr, addr_str, sizeof(addr_str));
    return std::string(addr_str);
}

void *prepare_and_bench_blocking(void *arg)
{
    struct dpdk_test_args *args = (struct dpdk_test_args *) arg;
    pthread_mutex_lock(&args->mutex);
    pthread_t t = pthread_self();
    set_cpu(t, args->cpu);
    args->started = true;
    pthread_cond_wait(args->cond, &args->mutex);
    auto& ctx = uhd::transport::uhd_dpdk_ctx::get();
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
        "48888",
        "48888",
        buff_args,
        dev_addr
    );

    bench(eth_data, 1, 0.1);
    return 0;
}

void prepare_and_bench_polling(void)
{
    auto& ctx = uhd::transport::uhd_dpdk_ctx::get();

    uhd::transport::dpdk_zero_copy::sptr eth_data[NUM_PORTS];
    uhd::transport::zero_copy_xport_params buff_args;
    buff_args.recv_frame_size = 8000;
    buff_args.send_frame_size = 8000;
    buff_args.num_send_frames = 8;
    buff_args.num_recv_frames = 8;
    auto dev_addr             = uhd::device_addr_t();
    eth_data[0] = uhd::transport::dpdk_zero_copy::make(
        ctx,
        0,
        get_ipv4_addr(1),
        "48888",
        "48888",
        buff_args,
        dev_addr
    );
    eth_data[1] = uhd::transport::dpdk_zero_copy::make(
        ctx,
        1,
        get_ipv4_addr(0),
        "48888",
        "48888",
        buff_args,
        dev_addr
    );

    bench(eth_data, NUM_PORTS, 0.0);
}

int main(int argc, char **argv)
{
    int retval, user0_cpu = 0, user1_cpu = 2;
    int status = 0;
    std::string args;
    std::string cpusets;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "UHD-DPDK args")
        ("polling-mode", "Use polling mode (single thread on own core)")
        ("cpusets", po::value<std::string>(&cpusets)->default_value(""), "which core(s) to use for a given thread in blocking mode (specify something like \"user0=0,user1=2\")")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

   auto dpdk_args = uhd::device_addr_t(args);

   auto cpuset_map = uhd::device_addr_t(cpusets);
    for (std::string& key : cpuset_map.keys()) {
        if (key == "user0") {
            user0_cpu = std::stoi(cpuset_map[key], NULL, 0);
        } else if (key == "user1") {
            user1_cpu = std::stoi(cpuset_map[key], NULL, 0);
        }
    }

    auto& ctx = uhd::transport::uhd_dpdk_ctx::get();
    ctx.init(args);

    if (vm.count("polling-mode")) {
        prepare_and_bench_polling();
    } else {
        pthread_cond_t cond;
        pthread_cond_init(&cond, NULL);
        struct dpdk_test_args bench_args[2];
        pthread_mutex_init(&bench_args[0].mutex, NULL);
        pthread_mutex_init(&bench_args[1].mutex, NULL);
        bench_args[0].cpu      = user0_cpu;
        bench_args[0].cond     = &cond;
        bench_args[0].dst_ip   = get_ipv4_addr(1);
        bench_args[0].started  = false;
        bench_args[0].portid   = 0;
        bench_args[1].cpu      = user1_cpu;
        bench_args[1].cond     = &cond;
        bench_args[1].dst_ip   = get_ipv4_addr(0);
        bench_args[1].started  = false;
        bench_args[1].portid   = 1;

        pthread_t threads[2];
        pthread_create(&threads[0], NULL, prepare_and_bench_blocking, &bench_args[0]);
        pthread_create(&threads[1], NULL, prepare_and_bench_blocking, &bench_args[1]);

        do {
            pthread_mutex_lock(&bench_args[0].mutex);
            if (bench_args[0].started)
                break;
            pthread_mutex_unlock(&bench_args[0].mutex);
        } while (true);
        pthread_mutex_unlock(&bench_args[0].mutex);

        do {
            pthread_mutex_lock(&bench_args[1].mutex);
            if (bench_args[1].started)
                break;
            pthread_mutex_unlock(&bench_args[1].mutex);
        } while (true);
        pthread_mutex_unlock(&bench_args[1].mutex);

        pthread_cond_broadcast(&cond);

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
