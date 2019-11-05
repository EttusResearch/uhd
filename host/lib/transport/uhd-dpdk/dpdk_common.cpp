//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <arpa/inet.h>
#include <rte_arp.h>
#include <boost/algorithm/string.hpp>

namespace uhd { namespace transport { namespace dpdk {

namespace {
constexpr uint64_t USEC                   = 1000000;
constexpr size_t DEFAULT_FRAME_SIZE       = 8000;
constexpr int DEFAULT_NUM_MBUFS           = 1024;
constexpr int DEFAULT_MBUF_CACHE_SIZE     = 315;
constexpr size_t DPDK_HEADERS_SIZE        = 14 + 20 + 8; // Ethernet + IPv4 + UDP
constexpr uint16_t DPDK_DEFAULT_RING_SIZE = 512;

inline char* eal_add_opt(
    std::vector<const char*>& argv, size_t n, char* dst, const char* opt, const char* arg)
{
    char* ptr = dst;
    strncpy(ptr, opt, n);
    argv.push_back(ptr);
    ptr += strlen(opt) + 1;
    n -= ptr - dst;
    strncpy(ptr, arg, n);
    argv.push_back(ptr);
    ptr += strlen(arg) + 1;
    return ptr;
}

inline std::string eth_addr_to_string(const struct ether_addr mac_addr)
{
    auto mac_stream = boost::format("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx");
    mac_stream % (uint32_t)mac_addr.addr_bytes[0] % (uint32_t)mac_addr.addr_bytes[1]
        % (uint32_t)mac_addr.addr_bytes[2] % (uint32_t)mac_addr.addr_bytes[3]
        % (uint32_t)mac_addr.addr_bytes[4] % (uint32_t)mac_addr.addr_bytes[5];
    return mac_stream.str();
}

inline void separate_ipv4_addr(
    const std::string ipv4, uint32_t& ipv4_addr, uint32_t& netmask)
{
    std::vector<std::string> result;
    boost::algorithm::split(
        result, ipv4, [](const char& in) { return in == '/'; }, boost::token_compress_on);
    UHD_ASSERT_THROW(result.size() == 2);
    ipv4_addr   = (uint32_t)inet_addr(result[0].c_str());
    int netbits = std::atoi(result[1].c_str());
    netmask     = htonl(0xffffffff << (32 - netbits));
}
} // namespace

dpdk_port::uptr dpdk_port::make(port_id_t port,
    size_t mtu,
    uint16_t num_queues,
    size_t num_mbufs,
    struct rte_mempool* rx_pktbuf_pool,
    struct rte_mempool* tx_pktbuf_pool,
    std::string ipv4_address)
{
    return std::make_unique<dpdk_port>(
        port, mtu, num_queues, num_mbufs, rx_pktbuf_pool, tx_pktbuf_pool, ipv4_address);
}

dpdk_port::dpdk_port(port_id_t port,
    size_t mtu,
    uint16_t num_queues,
    size_t num_mbufs,
    struct rte_mempool* rx_pktbuf_pool,
    struct rte_mempool* tx_pktbuf_pool,
    std::string ipv4_address)
    : _port(port)
    , _mtu(mtu)
    , _rx_pktbuf_pool(rx_pktbuf_pool)
    , _tx_pktbuf_pool(tx_pktbuf_pool)
{
    /* 1. Set MTU and IPv4 address */
    int retval;

    retval = rte_eth_dev_set_mtu(_port, _mtu);
    if (retval) {
        uint16_t actual_mtu;
        UHD_LOGGER_WARNING("DPDK")
            << boost::format("Port %d: Could not set mtu to %d") % _port % _mtu;
        rte_eth_dev_get_mtu(_port, &actual_mtu);
        UHD_LOGGER_WARNING("DPDK")
            << boost::format("Port %d: Current mtu=%d") % _port % _mtu;
        _mtu = actual_mtu;
    }

    separate_ipv4_addr(ipv4_address, _ipv4, _netmask);

    /* 2. Set hardware offloads */
    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(_port, &dev_info);
    uint64_t rx_offloads = DEV_RX_OFFLOAD_IPV4_CKSUM;
    uint64_t tx_offloads = DEV_TX_OFFLOAD_IPV4_CKSUM;
    if ((dev_info.rx_offload_capa & rx_offloads) != rx_offloads) {
        UHD_LOGGER_ERROR("DPDK") << boost::format("%d: Only supports RX offloads 0x%0llx")
                                        % _port % dev_info.rx_offload_capa;
        throw uhd::runtime_error("DPDK: Missing required RX offloads");
    }
    if ((dev_info.tx_offload_capa & tx_offloads) != tx_offloads) {
        UHD_LOGGER_ERROR("DPDK") << boost::format("%d: Only supports TX offloads 0x%0llx")
                                        % _port % dev_info.tx_offload_capa;
        throw uhd::runtime_error("DPDK: Missing required TX offloads");
    }

    // Check number of available queues
    if (dev_info.max_rx_queues < num_queues || dev_info.max_tx_queues < num_queues) {
        _num_queues = std::min(dev_info.max_rx_queues, dev_info.max_tx_queues);
        UHD_LOGGER_WARNING("DPDK")
            << boost::format("%d: Maximum queues supported is %d") % _port % _num_queues;
    } else {
        _num_queues = num_queues;
    }

    struct rte_eth_conf port_conf   = {};
    port_conf.rxmode.offloads       = rx_offloads | DEV_RX_OFFLOAD_JUMBO_FRAME;
    port_conf.rxmode.max_rx_pkt_len = _mtu;
    port_conf.txmode.offloads       = tx_offloads;

    retval = rte_eth_dev_configure(_port, _num_queues, _num_queues, &port_conf);
    if (retval != 0) {
        UHD_LOG_ERROR("DPDK", "Failed to configure the device");
        throw uhd::runtime_error("DPDK: Failed to configure the device");
    }

    /* 3. Set descriptor ring sizes */
    uint16_t rx_desc = num_mbufs;
    if (dev_info.rx_desc_lim.nb_max < rx_desc || dev_info.rx_desc_lim.nb_min > rx_desc
        || (dev_info.rx_desc_lim.nb_align - 1) & rx_desc) {
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("%d: %d RX descriptors requested, but must be in [%d,%d]")
                   % _port % num_mbufs % dev_info.rx_desc_lim.nb_min
                   % dev_info.rx_desc_lim.nb_max;
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("Num RX descriptors must also be aligned to 0x%x")
                   % dev_info.rx_desc_lim.nb_align;
        throw uhd::runtime_error("DPDK: Failed to allocate RX descriptors");
    }

    uint16_t tx_desc = num_mbufs;
    if (dev_info.tx_desc_lim.nb_max < tx_desc || dev_info.tx_desc_lim.nb_min > tx_desc
        || (dev_info.tx_desc_lim.nb_align - 1) & tx_desc) {
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("%d: %d TX descriptors requested, but must be in [%d,%d]")
                   % _port % num_mbufs % dev_info.tx_desc_lim.nb_min
                   % dev_info.tx_desc_lim.nb_max;
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("Num TX descriptors must also be aligned to 0x%x")
                   % dev_info.tx_desc_lim.nb_align;
        throw uhd::runtime_error("DPDK: Failed to allocate TX descriptors");
    }

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(_port, &rx_desc, &tx_desc);
    if (retval != 0) {
        UHD_LOG_ERROR("DPDK", "Failed to configure the DMA queues ");
        throw uhd::runtime_error("DPDK: Failed to configure the DMA queues");
    }

    /* 4. Set up the RX and TX DMA queues (May not be generally supported after
     * eth_dev_start) */
    unsigned int cpu_socket = rte_eth_dev_socket_id(_port);
    for (uint16_t i = 0; i < _num_queues; i++) {
        retval =
            rte_eth_rx_queue_setup(_port, i, rx_desc, cpu_socket, NULL, _rx_pktbuf_pool);
        if (retval < 0) {
            UHD_LOGGER_ERROR("DPDK")
                << boost::format("Port %d: Could not init RX queue %d") % _port % i;
            throw uhd::runtime_error("DPDK: Failure to init RX queue");
        }

        struct rte_eth_txconf txconf = dev_info.default_txconf;
        txconf.offloads              = DEV_TX_OFFLOAD_IPV4_CKSUM;
        retval = rte_eth_tx_queue_setup(_port, i, tx_desc, cpu_socket, &txconf);
        if (retval < 0) {
            UHD_LOGGER_ERROR("DPDK")
                << boost::format("Port %d: Could not init TX queue %d") % _port % i;
            throw uhd::runtime_error("DPDK: Failure to init TX queue");
        }
    }

    /* 5. Set up initial flow table */

    /* Append all free queues except 0, which is reserved for ARP */
    _free_queues.reserve(_num_queues - 1);
    for (unsigned int i = 1; i < _num_queues; i++) {
        _free_queues.push_back(i);
    }

    // struct rte_flow_attr flow_attr;
    // flow_attr.group = 0;
    // flow_attr.priority = 1;
    // flow_attr.ingress = 1;
    // flow_attr.egress = 0;
    // flow_attr.transfer = 0;
    // flow_attr.reserved = 0;

    // struct rte_flow_item[] flow_pattern = {
    //};
    // int rte_flow_validate(uint16_t port_id,
    //              const struct rte_flow_attr *attr,
    //              const struct rte_flow_item pattern[],
    //              const struct rte_flow_action actions[],
    //              struct rte_flow_error *error);
    // struct rte_flow * rte_flow_create(uint16_t port_id,
    //            const struct rte_flow_attr *attr,
    //            const struct rte_flow_item pattern[],
    //            const struct rte_flow_action *actions[],
    //            struct rte_flow_error *error);

    /* 6. Start the Ethernet device */
    retval = rte_eth_dev_start(_port);
    if (retval < 0) {
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("Port %d: Could not start device") % _port;
        throw uhd::runtime_error("DPDK: Failure to start device");
    }

    /* Grab and display the port MAC address. */
    rte_eth_macaddr_get(_port, &_mac_addr);
    UHD_LOGGER_TRACE("DPDK") << "Port " << _port
                             << " MAC: " << eth_addr_to_string(_mac_addr);
}

/* TODO: Do flow directions */
queue_id_t dpdk_port::alloc_queue(struct rte_flow_pattern recv_pattern[])
{
    std::lock_guard<std::mutex> lock(_mutex);
    UHD_ASSERT_THROW(_free_queues.size() != 0);
    auto queue = _free_queues.back();
    _free_queues.pop_back();
    return queue;
}

void dpdk_port::free_queue(queue_id_t queue)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto flow  = _flow_rules.at(queue);
    int status = rte_flow_destroy(_port, flow, NULL);
    if (status) {
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("Failed to destroy flow rule on port %u, queue %u") % _port
                   % queue;
        throw uhd::runtime_error("DPDK: Failed to destroy flow rule");
    } else {
        _flow_rules.erase(queue);
    }
    _free_queues.push_back(queue);
}

int dpdk_port::_arp_reply(struct rte_mempool* tx_pktbuf_pool, struct arp_hdr* arp_req)
{
    struct rte_mbuf* mbuf;
    struct ether_hdr* hdr;
    struct arp_hdr* arp_frame;

    mbuf = rte_pktmbuf_alloc(tx_pktbuf_pool);
    if (unlikely(mbuf == NULL)) {
        UHD_LOG_WARNING("DPDK", "Could not allocate packet buffer for ARP response");
        return -ENOMEM;
    }

    hdr       = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    arp_frame = (struct arp_hdr*)&hdr[1];

    ether_addr_copy(&arp_req->arp_data.arp_sha, &hdr->d_addr);
    ether_addr_copy(&_mac_addr, &hdr->s_addr);
    hdr->ether_type = rte_cpu_to_be_16(ETHER_TYPE_ARP);

    arp_frame->arp_hrd = rte_cpu_to_be_16(ARP_HRD_ETHER);
    arp_frame->arp_pro = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    arp_frame->arp_hln = 6;
    arp_frame->arp_pln = 4;
    arp_frame->arp_op  = rte_cpu_to_be_16(ARP_OP_REPLY);
    ether_addr_copy(&_mac_addr, &arp_frame->arp_data.arp_sha);
    arp_frame->arp_data.arp_sip = _ipv4;
    ether_addr_copy(&hdr->d_addr, &arp_frame->arp_data.arp_tha);
    arp_frame->arp_data.arp_tip = arp_req->arp_data.arp_sip;

    mbuf->pkt_len  = 42;
    mbuf->data_len = 42;

    // ARP replies always on queue 0
    if (rte_eth_tx_burst(_port, 0, &mbuf, 1) != 1) {
        UHD_LOGGER_WARNING("DPDK")
            << boost::format("%s: TX descriptor ring is full") % __func__;
        rte_pktmbuf_free(mbuf);
        return -EAGAIN;
    }
    return 0;
}

// TODO: ARP processing for queue 0
// int dpdk_port::process_arp(struct rte_mempool *tx_pktbuf_pool, struct arp_hdr
// *arp_frame)
//{
//    std::lock_guard<std::mutex> lock(_mutex);
//    uint32_t dest_ip = arp_frame->arp_data.arp_sip;
//    struct ether_addr dest_addr = arp_frame->arp_data.arp_sha;
//
//    /* Add entry to ARP table */
//    struct uhd_dpdk_arp_entry *entry = NULL;
//    rte_hash_lookup_data(_arp_table, &dest_ip, (void **) &entry);
//    if (!entry) {
//        entry = rte_zmalloc(NULL, sizeof(*entry), 0);
//        if (!entry) {
//            return -ENOMEM;
//        }
//        LIST_INIT(&entry->pending_list);
//        ether_addr_copy(&dest_addr, &entry->mac_addr);
//        if (rte_hash_add_key_data(port->arp_table, &dest_ip, entry) < 0) {
//            rte_free(entry);
//            return -ENOSPC;
//        }
//    } else {
//        struct uhd_dpdk_config_req *req = NULL;
//        ether_addr_copy(&dest_addr, &entry->mac_addr);
//        /* Now wake any config reqs waiting for the ARP */
//        LIST_FOREACH(req, &entry->pending_list, entry) {
//            _uhd_dpdk_config_req_compl(req, 0);
//        }
//        while (entry->pending_list.lh_first != NULL) {
//            LIST_REMOVE(entry->pending_list.lh_first, entry);
//        }
//    }
//    /* Respond if this was an ARP request */
//    if (arp_frame->arp_op == rte_cpu_to_be_16(ARP_OP_REQUEST) &&
//        arp_frame->arp_data.arp_tip == port->ipv4_addr) {
//        _arp_reply(tx_pktbuf_pool, arp_frame);
//    }
//
//    return 0;
//
//}

static dpdk_ctx* global_ctx = nullptr;
static std::mutex global_ctx_mutex;

dpdk_ctx::sptr dpdk_ctx::get()
{
    std::lock_guard<std::mutex> lock(global_ctx_mutex);
    if (!global_ctx) {
        auto new_ctx = std::make_shared<dpdk_ctx>();
        global_ctx   = new_ctx.get();
        return new_ctx;
    }
    return global_ctx->shared_from_this();
}

dpdk_ctx::dpdk_ctx(void) : _init_done(false) {}

dpdk_ctx::~dpdk_ctx(void)
{
    std::lock_guard<std::mutex> lock(global_ctx_mutex);
    global_ctx = nullptr;
    // Stop all the ports
    for (auto& port : _ports) {
        rte_eth_dev_stop(port.first);
    }
    // Free mempools
    for (auto& pool : _rx_pktbuf_pools) {
        rte_mempool_free(pool);
    }
    for (auto& pool : _tx_pktbuf_pools) {
        rte_mempool_free(pool);
    }
    // Free EAL resources
    rte_eal_cleanup();
}

void dpdk_ctx::_eal_init(const device_addr_t& eal_args)
{
    /* Build up argc and argv */
    std::vector<const char*> argv;
    argv.push_back("uhd::transport::dpdk");
    auto args = new std::array<char, 4096>();
    char* opt = args->data();
    char* end = args->data() + args->size();
    for (std::string& key : eal_args.keys()) {
        std::string val = eal_args[key];
        if (key == "dpdk_coremask") {
            opt = eal_add_opt(argv, end - opt, opt, "-c", val.c_str());
        } else if (key == "dpdk_corelist") {
            /* NOTE: This arg may have commas, so limited to config file */
            opt = eal_add_opt(argv, end - opt, opt, "-l", val.c_str());
        } else if (key == "dpdk_coremap") {
            opt = eal_add_opt(argv, end - opt, opt, "--lcores", val.c_str());
        } else if (key == "dpdk_master_lcore") {
            opt = eal_add_opt(argv, end - opt, opt, "--master-lcore", val.c_str());
        } else if (key == "dpdk_pci_blacklist") {
            opt = eal_add_opt(argv, end - opt, opt, "-b", val.c_str());
        } else if (key == "dpdk_pci_whitelist") {
            opt = eal_add_opt(argv, end - opt, opt, "-w", val.c_str());
        } else if (key == "dpdk_log_level") {
            opt = eal_add_opt(argv, end - opt, opt, "--log-level", val.c_str());
        } else if (key == "dpdk_huge_dir") {
            opt = eal_add_opt(argv, end - opt, opt, "--huge-dir", val.c_str());
        } else if (key == "dpdk_file_prefix") {
            opt = eal_add_opt(argv, end - opt, opt, "--file-prefix", val.c_str());
        } else if (key == "dpdk_driver") {
            opt = eal_add_opt(argv, end - opt, opt, "-d", val.c_str());
        }
        /* TODO: Change where log goes?
           int rte_openlog_stream( FILE * f)
         */
    }
    /* Init DPDK's EAL */
    int ret = rte_eal_init(argv.size(), (char**)argv.data());
    /* Done with the temporaries */
    delete args;

    if (ret < 0) {
        UHD_LOG_ERROR("DPDK", "Error with EAL initialization");
        throw uhd::runtime_error("Error with EAL initialization");
    }

    /* Create pktbuf pool entries, but only allocate on use  */
    int socket_count = rte_socket_count();
    for (int i = 0; i < socket_count; i++) {
        _rx_pktbuf_pools.push_back(NULL);
        _tx_pktbuf_pools.push_back(NULL);
    }
}

/**
 * Init DPDK environment, including DPDK's EAL.
 * This will make available information about the DPDK-assigned NIC devices.
 *
 * \param user_args User args passed in to override config files
 */
void dpdk_ctx::init(const device_addr_t& user_args)
{
    unsigned int i;
    std::lock_guard<std::mutex> lock(_init_mutex);
    if (!_init_done) {
        /* Gather global config, build args for EAL, and init UHD-DPDK */
        const device_addr_t dpdk_args = uhd::prefs::get_dpdk_args(user_args);
        UHD_LOG_TRACE("DPDK", "Configuration:" << std::endl << dpdk_args.to_pp_string());
        _eal_init(dpdk_args);

        /* TODO: Should MTU be defined per-port? */
        _mtu = dpdk_args.cast<size_t>("dpdk_mtu", DEFAULT_FRAME_SIZE);
        /* This is per queue */
        _num_mbufs = dpdk_args.cast<int>("dpdk_num_mbufs", DEFAULT_NUM_MBUFS);
        _mbuf_cache_size =
            dpdk_args.cast<int>("dpdk_mbuf_cache_size", DEFAULT_MBUF_CACHE_SIZE);

        /* Get device info for all the NIC ports */
        int num_dpdk_ports = rte_eth_dev_count_avail();
        UHD_ASSERT_THROW(num_dpdk_ports > 0);
        device_addrs_t nics(num_dpdk_ports);
        RTE_ETH_FOREACH_DEV(i)
        {
            struct ether_addr mac_addr;
            rte_eth_macaddr_get(i, &mac_addr);
            nics[i]["dpdk_mac"] = eth_addr_to_string(mac_addr);
        }

        /* Get user configuration for each NIC port */
        device_addrs_t args = separate_device_addr(user_args);
        size_t queue_count  = 0;
        RTE_ETH_FOREACH_DEV(i)
        {
            auto& nic = nics.at(i);
            for (const auto& arg : args) {
                /* Match DPDK-discovered NICs and user config via MAC addr */
                if (arg.has_key("dpdk_mac") && nic["dpdk_mac"] == arg["dpdk_mac"]) {
                    /* Copy user args for discovered NICs */
                    nic.update(arg, false);
                    break;
                }
            }
            /* Now combine user args with conf file */
            auto conf = uhd::prefs::get_dpdk_nic_args(nic);

            /* Update config, and remove ports that aren't fully configured */
            if (conf.has_key("dpdk_ipv4")) {
                nics[i] = conf;
                /* Update queue count, to generate a large enough mempool */
                queue_count += conf.cast<uint16_t>("dpdk_num_queues", rte_lcore_count());
            } else {
                nics[i] = device_addr_t();
            }
        }

        RTE_ETH_FOREACH_DEV(i)
        {
            auto& conf = nics.at(i);
            if (conf.has_key("dpdk_ipv4")) {
                // Allocating enough buffers for all DMA queues for each CPU socket
                // - This is a bit inefficient for larger systems, since NICs may not
                //   all be on one socket
                auto cpu_socket = rte_eth_dev_socket_id(i);
                auto rx_pool = _get_rx_pktbuf_pool(cpu_socket, _num_mbufs * queue_count);
                auto tx_pool = _get_tx_pktbuf_pool(cpu_socket, _num_mbufs * queue_count);
                UHD_LOG_TRACE("DPDK",
                    "Initializing NIC(" << i << "):" << std::endl
                                        << conf.to_pp_string());
                _ports[i] = dpdk_port::make(i,
                    _mtu,
                    conf.cast<uint16_t>("dpdk_num_queues", rte_lcore_count()),
                    _num_mbufs,
                    rx_pool,
                    tx_pool,
                    conf["dpdk_ipv4"]);
            }
        }

        UHD_LOG_TRACE("DPDK", "Waiting for links to come up...");
        rte_delay_ms(1000);
        for (auto& port : _ports) {
            struct rte_eth_link link;
            auto portid = port.second->get_port_id();
            rte_eth_link_get(portid, &link);
            unsigned int link_status = link.link_status;
            unsigned int link_speed  = link.link_speed;
            UHD_LOGGER_TRACE("DPDK") << boost::format("Port %u UP: %d, %u Mbps\n")
                                            % portid % link_status % link_speed;
        }
        UHD_LOG_TRACE("DPDK", "Init DONE!");

        _init_done = true;
    }
}

dpdk_port* dpdk_ctx::get_port(port_id_t port) const
{
    assert(is_init_done());
    if (_ports.count(port) == 0) {
        return nullptr;
    }
    return _ports.at(port).get();
}

dpdk_port* dpdk_ctx::get_port(struct ether_addr mac_addr) const
{
    assert(is_init_done());
    for (const auto& port : _ports) {
        struct ether_addr port_mac_addr;
        rte_eth_macaddr_get(port.first, &port_mac_addr);
        for (int j = 0; j < 6; j++) {
            if (mac_addr.addr_bytes[j] != port_mac_addr.addr_bytes[j]) {
                break;
            }
            if (j == 5) {
                return port.second.get();
            }
        }
    }
    return nullptr;
}

int dpdk_ctx::get_port_count(void)
{
    assert(is_init_done());
    return _ports.size();
}

int dpdk_ctx::get_port_queue_count(port_id_t portid)
{
    assert(is_init_done());
    return _ports.at(portid)->get_queue_count();
}

int dpdk_ctx::get_port_link_status(port_id_t portid) const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(portid, &link);
    return link.link_status;
}

int dpdk_ctx::get_route(const std::string& addr) const
{
    const uint32_t dst_ipv4 = (uint32_t)inet_addr(addr.c_str());
    for (const auto& port : _ports) {
        if (get_port_link_status(port.first) < 1)
            continue;
        uint32_t src_ipv4 = port.second->get_ipv4();
        uint32_t netmask  = port.second->get_netmask();
        if ((src_ipv4 & netmask) == (dst_ipv4 & netmask)) {
            return (int)port.first;
        }
    }
    return -ENODEV;
}


bool dpdk_ctx::is_init_done(void) const
{
    return _init_done.load();
}

struct rte_mempool* dpdk_ctx::_get_rx_pktbuf_pool(
    unsigned int cpu_socket, size_t num_bufs)
{
    if (!_rx_pktbuf_pools.at(cpu_socket)) {
        const int mbuf_size = _mtu + RTE_PKTMBUF_HEADROOM;
        char name[32];
        snprintf(name, sizeof(name), "rx_mbuf_pool_%u", cpu_socket);
        _rx_pktbuf_pools[cpu_socket] = rte_pktmbuf_pool_create(
            name, num_bufs, _mbuf_cache_size, 0, mbuf_size, SOCKET_ID_ANY);
        if (!_rx_pktbuf_pools.at(cpu_socket)) {
            UHD_LOG_ERROR("DPDK", "Could not allocate RX pktbuf pool");
            throw uhd::runtime_error("DPDK: Could not allocate RX pktbuf pool");
        }
    }
    return _rx_pktbuf_pools.at(cpu_socket);
}

struct rte_mempool* dpdk_ctx::_get_tx_pktbuf_pool(
    unsigned int cpu_socket, size_t num_bufs)
{
    if (!_tx_pktbuf_pools.at(cpu_socket)) {
        const int mbuf_size = _mtu + RTE_PKTMBUF_HEADROOM;
        char name[32];
        snprintf(name, sizeof(name), "tx_mbuf_pool_%u", cpu_socket);
        _tx_pktbuf_pools[cpu_socket] = rte_pktmbuf_pool_create(
            name, num_bufs, _mbuf_cache_size, 0, mbuf_size, SOCKET_ID_ANY);
        if (!_tx_pktbuf_pools.at(cpu_socket)) {
            UHD_LOG_ERROR("DPDK", "Could not allocate TX pktbuf pool");
            throw uhd::runtime_error("DPDK: Could not allocate TX pktbuf pool");
        }
    }
    return _tx_pktbuf_pools.at(cpu_socket);
}

}}} // namespace uhd::transport::dpdk
