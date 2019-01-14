//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "dpdk_zero_copy.hpp"
#include <uhd/config.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/uhd-dpdk.h>
#include <boost/make_shared.hpp>
#include <sys/syslog.h>
#include <stack>
#include <arpa/inet.h>

namespace uhd { namespace transport {

namespace {
    static constexpr uint64_t USEC = 1000000;
    // FIXME: Make configurable and have uhd-dpdk library track buffer sizes
    static constexpr size_t DEFAULT_FRAME_SIZE = 8000;

    inline char * eal_add_opt(std::vector<const char*>& argv, size_t n,
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
}

uhd_dpdk_ctx::uhd_dpdk_ctx(void) : _init_done(false) {}

uhd_dpdk_ctx::~uhd_dpdk_ctx(void) {}

/* Initialize uhd-dpdk (and do only once) */
void uhd_dpdk_ctx::init(const dict<std::string, std::string> &eal_args,
                        unsigned int num_ports, int *port_thread_mapping,
                        int num_mbufs, int mbuf_cache_size, size_t mtu)
{
    std::lock_guard<std::mutex> lock(_init_mutex);
    if (!_init_done) {
        _mtu = mtu;
        /* Build up argc and argv */
        std::vector<const char *> argv;
        argv.push_back("uhd-dpdk");
        char *args = new char[4096];
        char *opt = args;
        char *end = args + sizeof(args);
        for (std::string &key : eal_args.keys()) {
            std::string val = eal_args[key];
            if (key == "coremask") {
                opt = eal_add_opt(argv, end - opt, opt, "-c",
                                  val.c_str());
            } else if (key == "corelist") {
                /* NOTE: This arg may have commas, so limited to config file */
                opt = eal_add_opt(argv, end - opt, opt, "-l",
                                  val.c_str());
            } else if (key == "coremap") {
                opt = eal_add_opt(argv, end - opt, opt, "--lcores",
                                  val.c_str());
            } else if (key == "master-lcore") {
                opt = eal_add_opt(argv, end - opt, opt, "--master-lcore",
                                  val.c_str());
            } else if (key == "pci-blacklist") {
                opt = eal_add_opt(argv, end - opt, opt, "-b",
                                  val.c_str());
            } else if (key == "pci-whitelist") {
                opt = eal_add_opt(argv, end - opt, opt, "-w",
                                  val.c_str());
            } else if (key == "log-level") {
                opt = eal_add_opt(argv, end - opt, opt, "--log-level",
                                  val.c_str());
            } else if (key == "huge-dir") {
                opt = eal_add_opt(argv, end - opt, opt, "--huge-dir",
                                  val.c_str());
            } else if (key == "file-prefix") {
                opt = eal_add_opt(argv, end - opt, opt, "--file-prefix",
                                  val.c_str());
            }
        }
        uhd_dpdk_init(argv.size(), argv.data(), num_ports, port_thread_mapping, num_mbufs,
                      mbuf_cache_size, _mtu);
        delete args;
        _init_done = true;
    }
}

int uhd_dpdk_ctx::get_port_id(std::array<uint8_t, 6> mac_addr,
                              unsigned int &port_id)
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

bool uhd_dpdk_ctx::is_init_done(void)
{
    return _init_done.load();
}


class dpdk_zero_copy_msb : public managed_send_buffer {
public:
    dpdk_zero_copy_msb(struct uhd_dpdk_socket *sock,
                      std::stack<dpdk_zero_copy_msb *, std::vector<dpdk_zero_copy_msb *>> &free_bufs)
                      : _sock(sock), _buf(nullptr), _free_bufs(free_bufs) {};

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
                    DEFAULT_FRAME_SIZE);
    }

private:
    struct uhd_dpdk_socket *_sock;
    struct rte_mbuf *_buf;
    std::stack<dpdk_zero_copy_msb *, std::vector<dpdk_zero_copy_msb *>> &_free_bufs;
};

class dpdk_zero_copy_mrb : public managed_recv_buffer {
public:
    dpdk_zero_copy_mrb(struct uhd_dpdk_socket *sock,
                      std::stack<dpdk_zero_copy_mrb *, std::vector<dpdk_zero_copy_mrb *>> &free_bufs)
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
        if ((_buf->ol_flags & PKT_RX_IP_CKSUM_MASK) == PKT_RX_IP_CKSUM_BAD) {
            UHD_LOG_WARNING("DPDK", "IP checksum failure detected. Dropping packet...");
            uhd_dpdk_free_buf(_buf);
            _free_bufs.push(this);
            return sptr();
        }

        return make(this, uhd_dpdk_buf_to_data(_sock, _buf),
                    uhd_dpdk_get_len(_sock, _buf));
    }

private:
    struct uhd_dpdk_socket *_sock;
    struct rte_mbuf *_buf;
    std::stack<dpdk_zero_copy_mrb *, std::vector<dpdk_zero_copy_mrb *>> &_free_bufs;
};

class dpdk_zero_copy_impl : public dpdk_zero_copy {
public:

    dpdk_zero_copy_impl(struct uhd_dpdk_ctx &ctx,
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
        // TODO: Handle xport_params
        UHD_ASSERT_THROW(xport_params.recv_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.send_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.num_send_frames > 0);
        UHD_ASSERT_THROW(xport_params.num_recv_frames > 0);

        UHD_ASSERT_THROW(ctx.is_init_done());

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
            .local_port = src_port,
            .remote_port = dst_port,
            .dst_addr = dst_ipv4
        };
        _rx_sock = uhd_dpdk_sock_open(dpdk_port_id, UHD_DPDK_SOCK_UDP, &sockarg);
        UHD_ASSERT_THROW(_rx_sock != nullptr);

        // Backfill the local port, in case it was auto-assigned
        uhd_dpdk_udp_get_info(_rx_sock, &sockarg);

        sockarg.is_tx = true;
        sockarg.remote_port = dst_port;
        sockarg.dst_addr = dst_ipv4;
        _tx_sock = uhd_dpdk_sock_open(dpdk_port_id, UHD_DPDK_SOCK_UDP, &sockarg);
        UHD_ASSERT_THROW(_tx_sock != nullptr);

        // Create managed_buffer containers
        for (size_t i = 0; i < _num_recv_frames; i++) {
            _mrb_pool.push(new dpdk_zero_copy_mrb(_rx_sock, _mrb_pool));
        }
        for (size_t i = 0; i < _num_send_frames; i++) {
            _msb_pool.push(new dpdk_zero_copy_msb(_tx_sock, _msb_pool));
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
        return ntohs(sockarg.local_port);
    }

    std::string get_local_addr(void) const
    {
        uint32_t ipv4_addr;
        int status = uhd_dpdk_get_ipv4_addr(_port_id, &ipv4_addr, NULL);
        auto retval = std::to_string(ipv4_addr >> 0 & 0xff) +
                      "." +
                      std::to_string(ipv4_addr >> 8 & 0xff) +
                      "." +
                      std::to_string(ipv4_addr >> 16 & 0xff) +
                      "." +
                      std::to_string(ipv4_addr >> 24 & 0xff);
        return retval;
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
    struct uhd_dpdk_ctx &ctx,
    const unsigned int dpdk_port_id,
    const std::string &addr,
    const std::string &remote_port,
    const std::string &local_port,
    const zero_copy_xport_params &default_buff_args,
    const device_addr_t &hints)
{
    return dpdk_zero_copy::sptr(
        new dpdk_zero_copy_impl(ctx, dpdk_port_id, addr, remote_port, local_port, default_buff_args)
    );
}

}}
