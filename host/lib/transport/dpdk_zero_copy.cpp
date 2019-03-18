//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/uhd-dpdk.h>
#include <uhdlib/transport/dpdk_zero_copy.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <stack>
#include <sys/syslog.h>
#include <arpa/inet.h>

namespace uhd { namespace transport {

namespace {
constexpr uint64_t USEC = 1000000;
constexpr size_t DEFAULT_FRAME_SIZE = 8000;
constexpr int DEFAULT_NUM_MBUFS = 4095;
constexpr int DEFAULT_MBUF_CACHE_SIZE = 315;
constexpr size_t UHD_DPDK_HEADERS_SIZE = 14 + 20 + 8; // Ethernet + IPv4 + UDP

inline char * eal_add_opt(std::vector<const char*> &argv, size_t n,
    char *dst, const char *opt, const char *arg)
{
    char *ptr = dst;
    strncpy(ptr, opt, n);
    argv.push_back(ptr);
    ptr += strlen(opt) + 1;
    n -= ptr - dst;
    strncpy(ptr, arg, n);
    argv.push_back(ptr);
    ptr += strlen(arg) + 1;
    return ptr;
}

inline void uhd_dpdk_eal_init(const device_addr_t &eal_args)
{
    /* Build up argc and argv */
    std::vector<const char *> argv;
    argv.push_back("uhd-dpdk");
    auto args = new std::array<char, 4096>();
    char *opt = args->data();
    char *end = args->data() + args->size();
    for (std::string &key : eal_args.keys()) {
        std::string val = eal_args[key];
        if (key == "dpdk-coremask") {
            opt = eal_add_opt(argv, end - opt, opt, "-c",
                              val.c_str());
        } else if (key == "dpdk-corelist") {
            /* NOTE: This arg may have commas, so limited to config file */
            opt = eal_add_opt(argv, end - opt, opt, "-l",
                              val.c_str());
        } else if (key == "dpdk-coremap") {
            opt = eal_add_opt(argv, end - opt, opt, "--lcores",
                              val.c_str());
        } else if (key == "dpdk-master-lcore") {
            opt = eal_add_opt(argv, end - opt, opt, "--master-lcore",
                              val.c_str());
        } else if (key == "dpdk-pci-blacklist") {
            opt = eal_add_opt(argv, end - opt, opt, "-b",
                              val.c_str());
        } else if (key == "dpdk-pci-whitelist") {
            opt = eal_add_opt(argv, end - opt, opt, "-w",
                              val.c_str());
        } else if (key == "dpdk-log-level") {
            opt = eal_add_opt(argv, end - opt, opt, "--log-level",
                              val.c_str());
        } else if (key == "dpdk-huge-dir") {
            opt = eal_add_opt(argv, end - opt, opt, "--huge-dir",
                              val.c_str());
        } else if (key == "dpdk-file-prefix") {
            opt = eal_add_opt(argv, end - opt, opt, "--file-prefix",
                              val.c_str());
        } else if (key == "dpdk-driver") {
            opt = eal_add_opt(argv, end - opt, opt, "-d",
                              val.c_str());
        }
        /* TODO: Change where log goes?
           int rte_openlog_stream( FILE * f)
         */
    }
    /* Init DPDK's EAL */
    uhd_dpdk_init(argv.size(), argv.data());
    delete args;
}

inline std::string eth_addr_to_string(struct eth_addr mac_addr)
{
    auto mac_stream = boost::format("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx");
    mac_stream % (uint32_t) mac_addr.addr[0] % (uint32_t) mac_addr.addr[1]
               % (uint32_t) mac_addr.addr[2] % (uint32_t) mac_addr.addr[3]
               % (uint32_t) mac_addr.addr[4] % (uint32_t) mac_addr.addr[5];
    return mac_stream.str();
}

inline void separate_ipv4_addr(const std::string ipv4,
    uint32_t &ipv4_addr, uint32_t &netmask)
{
    std::vector<std::string> result;
    boost::algorithm::split(result, ipv4,
        [](const char &in) {return in == '/';}, boost::token_compress_on);
    UHD_ASSERT_THROW(result.size() == 2);
    ipv4_addr = (uint32_t) inet_addr(result[0].c_str());
    int netbits = std::atoi(result[1].c_str());
    netmask = htonl(0xffffffff << (32-netbits));
}
} // namespace

uhd_dpdk_ctx::uhd_dpdk_ctx(void) : _init_done(false) {}

uhd_dpdk_ctx::~uhd_dpdk_ctx(void) {}

/* Initialize uhd-dpdk (and do only once) */
void uhd_dpdk_ctx::init(const device_addr_t &user_args)
{
    std::lock_guard<std::mutex> lock(_init_mutex);
    if (!_init_done) {
        /* Gather global config, build args for EAL, and init UHD-DPDK */
        const device_addr_t dpdk_args = uhd::prefs::get_dpdk_args(user_args);
        UHD_LOG_TRACE("DPDK", "Configuration:" << std::endl
            << dpdk_args.to_pp_string());
        uhd_dpdk_eal_init(dpdk_args);

        _mtu = dpdk_args.has_key("dpdk-mtu")
            ? dpdk_args.cast<size_t>("dpdk-mtu", 0)
            : DEFAULT_FRAME_SIZE;
        const int num_mbufs = dpdk_args.has_key("dpdk-num-mbufs")
            ? dpdk_args.cast<int>("dpdk-num-mbufs", 0)
            : DEFAULT_NUM_MBUFS;
        const int mbuf_cache_size = dpdk_args.has_key("dpdk-mbuf-cache-size")
            ? dpdk_args.cast<int>("dpdk-mbuf-cache-size", 0)
            : DEFAULT_MBUF_CACHE_SIZE;

        /* Get configuration for all the NIC ports */
        device_addrs_t args = separate_device_addr(user_args);
        int num_ports = uhd_dpdk_port_count();
        std::vector<int> io_cpu_map(num_ports);
        device_addrs_t nics(num_ports);
        for (ssize_t i = 0; i < num_ports; i++) {
            struct eth_addr mac_addr = uhd_dpdk_get_eth_addr(i);
            nics[i]["dpdk-mac"] = eth_addr_to_string(mac_addr);
            for (const auto &arg: args) {
                if (arg.has_key("dpdk-mac")
                    && arg["dpdk-mac"] == nics[i]["dpdk-mac"]) {
                    for (const auto& key: arg.keys()) {
                        nics[i][key] = arg[key];
                    }
                    break;
                }
            }
            nics[i] = uhd::prefs::get_dpdk_nic_args(nics[i]);
            if (nics[i].has_key("dpdk-ipv4")
                && nics[i].has_key("dpdk-io-cpu")) {
                uint32_t ipv4_addr, netmask;
                io_cpu_map[i] = std::atoi(nics[i]["dpdk-io-cpu"].c_str());
                separate_ipv4_addr(nics[i]["dpdk-ipv4"], ipv4_addr, netmask);
                uhd_dpdk_set_ipv4_addr((unsigned int) i, ipv4_addr, netmask);
            } else {
                /* Not enough configuration to use NIC */
                io_cpu_map[i] = -1;
            }
            UHD_LOG_TRACE("DPDK", "Found NIC(" << i << "):" << std::endl
                << nics[i].to_pp_string());
        }
        uhd_dpdk_start(num_ports, io_cpu_map.data(), num_mbufs,
            mbuf_cache_size, _mtu);
        _init_done = true;
    }
}

size_t uhd_dpdk_ctx::get_mtu(void) const
{
    UHD_ASSERT_THROW(is_init_done());
    return _mtu;
}

int uhd_dpdk_ctx::get_port_id(std::array<uint8_t, 6> mac_addr,
    unsigned int &port_id) const
{
    UHD_ASSERT_THROW(is_init_done());
    int num_ports = uhd_dpdk_port_count();
    for (int i = 0; i < num_ports; i++) {
        struct eth_addr port_mac_addr = uhd_dpdk_get_eth_addr((unsigned int) i);
        for (int j = 0; j < 6; j++) {
            if (mac_addr[j] != port_mac_addr.addr[j]) {
                break;
            }
            if (j == 5) {
                port_id = (unsigned int) i;
                return 0;
            }
        }
    }
    return -1;
}

int uhd_dpdk_ctx::get_route(const std::string &addr) const
{
    const uint32_t dst_ipv4 = (uint32_t) inet_addr(addr.c_str());
    const unsigned int num_ports = uhd_dpdk_port_count();
    for (unsigned int port = 0; port < num_ports; port++) {
        uint32_t src_ipv4;
        uint32_t netmask;
        if (uhd_dpdk_port_link_status(port) < 1)
            continue;
        uhd_dpdk_get_ipv4_addr(port, &src_ipv4, &netmask);
        if ((src_ipv4 & netmask) == (dst_ipv4 & netmask)) {
            return (int) port;
	}
    }
    return -ENODEV;
}

int uhd_dpdk_ctx::set_ipv4_addr(unsigned int port_id, uint32_t ipv4_addr,
    uint32_t netmask)
{
    return uhd_dpdk_set_ipv4_addr(port_id, ipv4_addr, netmask);
}

bool uhd_dpdk_ctx::is_init_done(void) const
{
    return _init_done.load();
}


class dpdk_zero_copy_msb : public managed_send_buffer {
public:
    dpdk_zero_copy_msb(struct uhd_dpdk_socket *sock,
                       std::stack<dpdk_zero_copy_msb *, std::vector<dpdk_zero_copy_msb *>> &free_bufs,
                       size_t frame_size)
                      : _sock(sock), _buf(nullptr), _free_bufs(free_bufs),
                        _frame_size(frame_size) {};

    ~dpdk_zero_copy_msb(void) {}

    void release(void)
    {
        if (_buf) {
            _buf->pkt_len = _length;
            _buf->data_len = _length;
            int num_tx = uhd_dpdk_send(_sock, &_buf, 1);
            if (num_tx == 0) {
                /* Drop packet and free buffer (do not share sockets!) */
                UHD_LOG_ERROR("DPDK", "Invalid shared socket usage detected. Dropping packet...");
                uhd_dpdk_free_buf(_buf);
            }
            // Push back into pool
            _free_bufs.push(this);
        }
    }

    sptr get_new(double timeout)
    {
        int bufs = uhd_dpdk_request_tx_bufs(_sock, &_buf, 1, timeout);
        if (bufs != 1 || !_buf)
            return sptr();

        return make(this, uhd_dpdk_buf_to_data(_sock, _buf),
                    _frame_size);
    }

private:
    struct uhd_dpdk_socket *_sock;
    struct rte_mbuf *_buf;
    std::stack<dpdk_zero_copy_msb *, std::vector<dpdk_zero_copy_msb *>> &_free_bufs;
    size_t _frame_size;
};

class dpdk_zero_copy_mrb : public managed_recv_buffer {
public:
    dpdk_zero_copy_mrb(struct uhd_dpdk_socket *sock,
        std::stack<dpdk_zero_copy_mrb*, std::vector<dpdk_zero_copy_mrb*>> &free_bufs)
        : _sock(sock), _buf(nullptr), _free_bufs(free_bufs) {};
    ~dpdk_zero_copy_mrb(void) {}

    void release(void)
    {
        if (_buf) {
            uhd_dpdk_free_buf(_buf);
            _free_bufs.push(this);
        }
    }

    sptr get_new(double timeout)
    {
        int bufs = uhd_dpdk_recv(_sock, &_buf, 1, (int) (timeout*USEC));
        if (bufs != 1 || _buf == nullptr) {
            // Push back into pool if we didn't get a real buffer
            _free_bufs.push(this);
            return sptr();
        }

        return make(this, uhd_dpdk_buf_to_data(_sock, _buf),
                    uhd_dpdk_get_len(_sock, _buf));
    }

private:
    struct uhd_dpdk_socket *_sock;
    struct rte_mbuf *_buf;
    std::stack<dpdk_zero_copy_mrb*, std::vector<dpdk_zero_copy_mrb*>> &_free_bufs;
};

class dpdk_zero_copy_impl : public dpdk_zero_copy {
public:

    dpdk_zero_copy_impl(const struct uhd_dpdk_ctx &ctx,
                        const unsigned int dpdk_port_id,
                        const std::string &addr,
                        const std::string &remote_port,
                        const std::string &local_port,
                        const zero_copy_xport_params& xport_params)
                          : _num_send_frames(xport_params.num_send_frames),
                            _send_frame_size(xport_params.send_frame_size),
                            _num_recv_frames(xport_params.num_recv_frames),
                            _recv_frame_size(xport_params.recv_frame_size),
                            _port_id(dpdk_port_id),
                            _rx_empty_count(0),
                            _tx_empty_count(0)
    {
        UHD_ASSERT_THROW(xport_params.recv_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.send_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.num_send_frames > 0);
        UHD_ASSERT_THROW(xport_params.num_recv_frames > 0);

        UHD_ASSERT_THROW(ctx.is_init_done());
        UHD_ASSERT_THROW(xport_params.recv_frame_size < ctx.get_mtu() - UHD_DPDK_HEADERS_SIZE);
        UHD_ASSERT_THROW(xport_params.send_frame_size < ctx.get_mtu() - UHD_DPDK_HEADERS_SIZE);

        const int num_ports = uhd_dpdk_port_count();
        UHD_ASSERT_THROW(num_ports > 0);
        UHD_ASSERT_THROW(dpdk_port_id < (unsigned int) num_ports);

        // Convert ipv4 addr from string to uint32_t, network format
        uint32_t dst_ipv4 = (uint32_t) inet_addr(addr.c_str());
        // Convert port from string to uint16_t, network format
        uint16_t dst_port = htons(std::stoi(remote_port, NULL, 0));
        uint16_t src_port = htons(std::stoi(local_port, NULL, 0));

        // Create RX socket first
        struct uhd_dpdk_sockarg_udp sockarg = {
            .is_tx = false,
            .filter_bcast = true,
            .local_port = src_port,
            .remote_port = dst_port,
            .dst_addr = dst_ipv4,
            .num_bufs = _num_recv_frames
        };
        _rx_sock = uhd_dpdk_sock_open(dpdk_port_id, UHD_DPDK_SOCK_UDP, &sockarg);
        UHD_ASSERT_THROW(_rx_sock != nullptr);

        // Backfill the local port, in case it was auto-assigned
        uhd_dpdk_udp_get_info(_rx_sock, &sockarg);

        sockarg.is_tx = true;
        sockarg.num_bufs = _num_send_frames;
        sockarg.remote_port = dst_port;
        sockarg.dst_addr = dst_ipv4;
        _tx_sock = uhd_dpdk_sock_open(dpdk_port_id, UHD_DPDK_SOCK_UDP, &sockarg);
        UHD_ASSERT_THROW(_tx_sock != nullptr);

        // Create managed_buffer containers
        for (size_t i = 0; i < _num_recv_frames; i++) {
            _mrb_pool.push(new dpdk_zero_copy_mrb(_rx_sock, _mrb_pool));
        }
        for (size_t i = 0; i < _num_send_frames; i++) {
            _msb_pool.push(new dpdk_zero_copy_msb(_tx_sock, _msb_pool, _send_frame_size));
        }

        UHD_LOG_TRACE("DPDK", "Created transports between " << addr << ":"
                               << remote_port << " and NIC(" << dpdk_port_id
                               << "):" << ntohs(sockarg.local_port));
    }

    ~dpdk_zero_copy_impl(void)
    {
        struct uhd_dpdk_sockarg_udp sockarg;
        size_t count;
        uhd_dpdk_udp_get_info(_rx_sock, &sockarg);
        uhd_dpdk_get_drop_count(_rx_sock, &count);
        UHD_LOG_TRACE("DPDK", "Closing transports between " << sockarg.dst_addr << ":"
                               << ntohs(sockarg.remote_port) << " and local:"
                               << ntohs(sockarg.local_port));
        UHD_LOG_TRACE("DPDK", "(" << ntohs(sockarg.remote_port) << "," << ntohs(sockarg.local_port) << ") "
                              << " Dropped "<< count << " packets");
        uhd_dpdk_get_xfer_count(_rx_sock, &count);
        UHD_LOG_TRACE("DPDK", "(" << ntohs(sockarg.remote_port) << "," << ntohs(sockarg.local_port) << ") "
                              << " Received "<< count << " packets");
        UHD_LOG_TRACE("DPDK", "(" << ntohs(sockarg.remote_port) << "," << ntohs(sockarg.local_port) << ") "
                              << "RX empty count is " << _rx_empty_count);
        UHD_LOG_TRACE("DPDK", "(" << ntohs(sockarg.remote_port) << "," << ntohs(sockarg.local_port) << ") "
                              << "TX empty count is " << _tx_empty_count);
        uhd_dpdk_sock_close(_rx_sock);
        uhd_dpdk_sock_close(_tx_sock);
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout = 0.1)
    {
        if (_mrb_pool.empty()) {
            _rx_empty_count++;
            return managed_recv_buffer::sptr();
        }

        dpdk_zero_copy_mrb *mrb = _mrb_pool.top();
        _mrb_pool.pop();
        managed_recv_buffer::sptr buff = mrb->get_new(timeout);
        if (!buff)
            _rx_empty_count++;
        return buff;
    }

    size_t get_num_recv_frames(void) const
    {
        return _num_recv_frames;
    }

    size_t get_recv_frame_size(void) const
    {
        return _recv_frame_size;
    }

    managed_send_buffer::sptr get_send_buff(double timeout = 0.1)
    {
        if (_msb_pool.empty()) {
            _tx_empty_count++;
            return managed_send_buffer::sptr();
        }

        dpdk_zero_copy_msb *msb = _msb_pool.top();
        _msb_pool.pop();
        managed_send_buffer::sptr buff = msb->get_new(timeout);
        if (!buff)
            _tx_empty_count++;
        return buff;
    }

    size_t get_num_send_frames(void) const
    {
        return _num_send_frames;
    }

    size_t get_send_frame_size(void) const
    {
        return _send_frame_size;
    }

    uint16_t get_local_port(void) const
    {
        struct uhd_dpdk_sockarg_udp sockarg;
        int status = uhd_dpdk_udp_get_info(_rx_sock, &sockarg);
        UHD_ASSERT_THROW(status == 0);
        return ntohs(sockarg.local_port);
    }

    std::string get_local_addr(void) const
    {
        struct in_addr ipv4_addr;
        int status = uhd_dpdk_get_ipv4_addr(_port_id, &ipv4_addr.s_addr, NULL);
        UHD_ASSERT_THROW(status == 0);
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipv4_addr, addr_str, sizeof(addr_str));
        return std::string(addr_str);
    }

    uint32_t get_drop_count(void) const
    {
        size_t drop_count = 0;
        uhd_dpdk_get_drop_count(_rx_sock, &drop_count);
        return drop_count;
    }
private:
    struct uhd_dpdk_socket *_rx_sock;
    struct uhd_dpdk_socket *_tx_sock;
    const size_t _num_send_frames;
    const size_t _send_frame_size;
    const size_t _num_recv_frames;
    const size_t _recv_frame_size;
    const unsigned int _port_id;
    unsigned int _rx_empty_count;
    unsigned int _tx_empty_count;

    std::stack<dpdk_zero_copy_mrb *, std::vector<dpdk_zero_copy_mrb *>> _mrb_pool;
    std::stack<dpdk_zero_copy_msb *, std::vector<dpdk_zero_copy_msb *>> _msb_pool;
};

dpdk_zero_copy::sptr dpdk_zero_copy::make(
    const struct uhd_dpdk_ctx &ctx,
    const unsigned int dpdk_port_id,
    const std::string &addr,
    const std::string &remote_port,
    const std::string &local_port,
    const zero_copy_xport_params &default_buff_args,
    const device_addr_t &/*hints*/)
{
    return dpdk_zero_copy::sptr(
        new dpdk_zero_copy_impl(ctx, dpdk_port_id, addr, remote_port, local_port, default_buff_args)
    );
}

}} // namespace uhd::transport
