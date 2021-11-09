//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/utils/thread.hpp>
#include <uhdlib/transport/dpdk/arp.hpp>
#include <uhdlib/transport/dpdk/udp.hpp>
#include <uhdlib/transport/dpdk_io_service_client.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <cmath>

/*
 * Memory management
 *
 * Every object that allocates and frees DPDK memory has a reference to the
 * dpdk_ctx.
 *
 * Ownership hierarchy:
 *
 * dpdk_io_service_mgr (1) =>
 *     dpdk_ctx::sptr
 *     dpdk_io_service::sptr
 *
 * xport (1) =>
 *     dpdk_send_io::sptr
 *     dpdk_recv_io::sptr
 *
 * usrp link_mgr (1) =>
 *     udp_dpdk_link::sptr
 *
 * dpdk_send_io (2) =>
 *     dpdk_ctx::sptr
 *     dpdk_io_service::sptr
 *
 * dpdk_recv_io (2) =>
 *     dpdk_ctx::sptr
 *     dpdk_io_service::sptr
 *
 * dpdk_io_service (3) =>
 *     dpdk_ctx::wptr (weak_ptr)
 *     udp_dpdk_link::sptr
 *
 * udp_dpdk_link (4) =>
 *     dpdk_ctx::sptr
 */

using namespace uhd::transport;

dpdk_io_service::dpdk_io_service(
    unsigned int lcore_id, std::vector<dpdk::dpdk_port*> ports, size_t servq_depth)
    : _ctx(dpdk::dpdk_ctx::get())
    , _lcore_id(lcore_id)
    , _ports(ports)
    , _servq(servq_depth, lcore_id)
{
    UHD_LOG_TRACE("DPDK::IO_SERVICE", "Launching I/O service for lcore " << lcore_id);
    for (auto port : _ports) {
        UHD_LOG_TRACE("DPDK::IO_SERVICE",
            "lcore_id " << lcore_id << ": Adding port index " << port->get_port_id());
        _tx_queues[port->get_port_id()]      = std::list<dpdk_send_io*>();
        _recv_xport_map[port->get_port_id()] = std::list<dpdk_recv_io*>();
    }
    int status = rte_eal_remote_launch(_io_worker, this, lcore_id);
    if (status) {
        throw uhd::runtime_error("DPDK: I/O service cannot launch on busy lcore");
    }
}

dpdk_io_service::sptr dpdk_io_service::make(
    unsigned int lcore_id, std::vector<dpdk::dpdk_port*> ports, size_t servq_depth)
{
    return dpdk_io_service::sptr(new dpdk_io_service(lcore_id, ports, servq_depth));
}

dpdk_io_service::~dpdk_io_service()
{
    UHD_LOG_TRACE(
        "DPDK::IO_SERVICE", "Shutting down I/O service for lcore " << _lcore_id);
    dpdk::wait_req* req = dpdk::wait_req_alloc(dpdk::wait_type::WAIT_LCORE_TERM, NULL);
    if (!req) {
        UHD_LOG_ERROR("DPDK::IO_SERVICE",
            "Could not allocate request for lcore termination for lcore " << _lcore_id);
        return;
    }
    dpdk::wait_req_get(req);
    _servq.submit(req, std::chrono::microseconds(-1));
    dpdk::wait_req_put(req);
}

void dpdk_io_service::attach_recv_link(recv_link_if::sptr link)
{
    struct dpdk_flow_data data;
    data.link    = dynamic_cast<udp_dpdk_link*>(link.get());
    data.is_recv = true;
    assert(data.link);
    auto req = wait_req_alloc(dpdk::wait_type::WAIT_FLOW_OPEN, (void*)&data);
    if (!req) {
        UHD_LOG_ERROR(
            "DPDK::IO_SERVICE", "Could not allocate wait_req to attach recv_link");
        throw uhd::runtime_error("DPDK: Could not allocate wait_req to attach recv_link");
    }
    _servq.submit(req, std::chrono::microseconds(-1));
    dpdk::wait_req_put(req);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _recv_links.push_back(link);
    }
}

void dpdk_io_service::attach_send_link(send_link_if::sptr link)
{
    udp_dpdk_link* dpdk_link = dynamic_cast<udp_dpdk_link*>(link.get());
    assert(dpdk_link);

    // First, fill in destination MAC address
    struct dpdk::arp_request arp_data;
    arp_data.tpa  = dpdk_link->get_remote_ipv4();
    arp_data.port = dpdk_link->get_port()->get_port_id();
    if (dpdk_link->get_port()->dst_is_broadcast(arp_data.tpa)) {
        // If a broadcast IP, skip the ARP and fill with broadcast MAC addr
        memset(arp_data.tha.addr_bytes, 0xFF, 6);
    } else {
        auto arp_req = wait_req_alloc(dpdk::wait_type::WAIT_ARP, (void*)&arp_data);
        if (!arp_req) {
            UHD_LOG_ERROR(
                "DPDK::IO_SERVICE", "Could not allocate wait_req for ARP request");
            throw uhd::runtime_error("DPDK: Could not allocate wait_req for ARP request");
        }
        if (_servq.submit(arp_req, std::chrono::microseconds(3000000))) {
            // Try one more time...
            auto arp_req2 = wait_req_alloc(dpdk::wait_type::WAIT_ARP, (void*)&arp_data);
            if (_servq.submit(arp_req2, std::chrono::microseconds(30000000))) {
                wait_req_put(arp_req);
                wait_req_put(arp_req2);
                throw uhd::io_error("DPDK: Could not reach host");
            }
            wait_req_put(arp_req2);
        }
        wait_req_put(arp_req);
    }
    dpdk_link->set_remote_mac(arp_data.tha);

    // Then, submit the link to the I/O service thread
    struct dpdk_flow_data data;
    data.link    = dpdk_link;
    data.is_recv = false;
    auto req     = wait_req_alloc(dpdk::wait_type::WAIT_FLOW_OPEN, (void*)&data);
    if (!req) {
        UHD_LOG_ERROR(
            "DPDK::IO_SERVICE", "Could not allocate wait_req to attach send_link");
        throw uhd::runtime_error("DPDK: Could not allocate wait_req to attach send_link");
    }
    _servq.submit(req, std::chrono::microseconds(-1));
    wait_req_put(req);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _send_links.push_back(link);
    }
}

void dpdk_io_service::detach_recv_link(recv_link_if::sptr link)
{
    auto link_ptr = link.get();
    struct dpdk_flow_data data;
    data.link    = dynamic_cast<udp_dpdk_link*>(link_ptr);
    data.is_recv = true;
    auto req     = wait_req_alloc(dpdk::wait_type::WAIT_FLOW_CLOSE, (void*)&data);
    if (!req) {
        UHD_LOG_ERROR(
            "DPDK::IO_SERVICE", "Could not allocate wait_req to detach recv_link");
        throw uhd::runtime_error("DPDK: Could not allocate wait_req to detach recv_link");
    }
    _servq.submit(req, std::chrono::microseconds(-1));
    wait_req_put(req);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _recv_links.remove_if(
            [link_ptr](recv_link_if::sptr& item) { return item.get() == link_ptr; });
    }
}

void dpdk_io_service::detach_send_link(send_link_if::sptr link)
{
    auto link_ptr = link.get();
    struct dpdk_flow_data data;
    data.link    = dynamic_cast<udp_dpdk_link*>(link_ptr);
    data.is_recv = false;
    auto req     = wait_req_alloc(dpdk::wait_type::WAIT_FLOW_CLOSE, (void*)&data);
    if (!req) {
        UHD_LOG_ERROR(
            "DPDK::IO_SERVICE", "Could not allocate wait_req to detach send_link");
        throw uhd::runtime_error("DPDK: Could not allocate wait_req to detach send_link");
    }
    _servq.submit(req, std::chrono::microseconds(-1));
    wait_req_put(req);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _send_links.remove_if(
            [link_ptr](send_link_if::sptr& item) { return item.get() == link_ptr; });
    }
}

recv_io_if::sptr dpdk_io_service::make_recv_client(recv_link_if::sptr data_link,
    size_t num_recv_frames,
    recv_callback_t cb,
    send_link_if::sptr /*fc_link*/,
    size_t num_send_frames,
    recv_io_if::fc_callback_t fc_cb)
{
    auto link    = dynamic_cast<udp_dpdk_link*>(data_link.get());
    auto recv_io = std::make_shared<dpdk_recv_io>(
        shared_from_this(), link, num_recv_frames, cb, num_send_frames, fc_cb);

    // Register with I/O service
    recv_io->_dpdk_io_if.io_client = static_cast<void*>(recv_io.get());
    auto xport_req                 = dpdk::wait_req_alloc(
        dpdk::wait_type::WAIT_XPORT_CONNECT, (void*)&recv_io->_dpdk_io_if);
    _servq.submit(xport_req, std::chrono::microseconds(-1));
    wait_req_put(xport_req);
    return recv_io;
}

send_io_if::sptr dpdk_io_service::make_send_client(send_link_if::sptr send_link,
    size_t num_send_frames,
    send_io_if::send_callback_t send_cb,
    recv_link_if::sptr /*recv_link*/,
    size_t num_recv_frames,
    recv_callback_t recv_cb,
    send_io_if::fc_callback_t fc_cb)
{
    auto link    = dynamic_cast<udp_dpdk_link*>(send_link.get());
    auto send_io = std::make_shared<dpdk_send_io>(shared_from_this(),
        link,
        num_send_frames,
        send_cb,
        num_recv_frames,
        recv_cb,
        fc_cb);

    // Register with I/O service
    send_io->_dpdk_io_if.io_client = static_cast<void*>(send_io.get());
    auto xport_req                 = dpdk::wait_req_alloc(
        dpdk::wait_type::WAIT_XPORT_CONNECT, (void*)&send_io->_dpdk_io_if);
    _servq.submit(xport_req, std::chrono::microseconds(-1));
    wait_req_put(xport_req);
    return send_io;
}


int dpdk_io_service::_io_worker(void* arg)
{
    if (!arg)
        return -EINVAL;
    dpdk_io_service* srv = (dpdk_io_service*)arg;

    /* Check that this is a valid lcore */
    unsigned int lcore_id = rte_lcore_id();
    if (lcore_id == LCORE_ID_ANY)
        return -ENODEV;

    /* Check that this lcore has ports */
    if (srv->_ports.size() == 0)
        return -ENODEV;

    char name[16];
    snprintf(name, sizeof(name), "dpdk-io_%hu", (uint16_t)lcore_id);
    rte_thread_setname(pthread_self(), name);
    UHD_LOG_TRACE("DPDK::IO_SERVICE",
        "I/O service thread '" << name << "' started on lcore " << lcore_id);

    uhd::set_thread_priority_safe();

    snprintf(name, sizeof(name), "rx-tbl_%hu", (uint16_t)lcore_id);
    struct rte_hash_parameters hash_params = {.name = name,
        .entries                                    = MAX_FLOWS,
        .reserved                                   = 0,
        .key_len                                    = sizeof(struct dpdk::ipv4_5tuple),
        .hash_func                                  = NULL,
        .hash_func_init_val                         = 0,
        .socket_id  = uhd::narrow_cast<int>(rte_socket_id()),
        .extra_flag = 0};
    srv->_rx_table                         = rte_hash_create(&hash_params);
    if (srv->_rx_table == NULL) {
        return rte_errno;
    }

    int status = 0;
    while (!status) {
        /* For each port, attempt to receive packets and process */
        for (auto port : srv->_ports) {
            srv->_rx_burst(port, 0);
        }
        /* For each port's TX queues, do TX */
        for (auto port : srv->_ports) {
            srv->_tx_burst(port);
        }
        /* For each port's RX release queues, release buffers */
        for (auto port : srv->_ports) {
            srv->_rx_release(port);
        }
        /* Retry waking clients */
        if (srv->_retry_head) {
            dpdk_io_if* node = srv->_retry_head;
            dpdk_io_if* end  = srv->_retry_head->prev;
            while (true) {
                dpdk_io_if* next = node->next;
                srv->_wake_client(node);
                if (node == end) {
                    break;
                } else {
                    node = next;
                    next = node->next;
                }
            }
        }
        /* Check for open()/close()/term() requests and service 1 at a time
         * Leave this last so we immediately terminate if requested
         */
        status = srv->_service_requests();
    }

    return status;
}

int dpdk_io_service::_service_requests()
{
    for (int i = 0; i < MAX_PENDING_SERVICE_REQS; i++) {
        /* Dequeue */
        dpdk::wait_req* req = _servq.pop();
        if (!req) {
            break;
        }
        switch (req->reason) {
            case dpdk::wait_type::WAIT_SIMPLE:
                while (_servq.complete(req) == -ENOBUFS)
                    ;
                break;
            case dpdk::wait_type::WAIT_RX:
            case dpdk::wait_type::WAIT_TX_BUF:
                throw uhd::not_implemented_error(
                    "DPDK: _service_requests(): DPDK is still a WIP");
            case dpdk::wait_type::WAIT_FLOW_OPEN:
                _service_flow_open(req);
                break;
            case dpdk::wait_type::WAIT_FLOW_CLOSE:
                _service_flow_close(req);
                break;
            case dpdk::wait_type::WAIT_XPORT_CONNECT:
                _service_xport_connect(req);
                break;
            case dpdk::wait_type::WAIT_XPORT_DISCONNECT:
                _service_xport_disconnect(req);
                break;
            case dpdk::wait_type::WAIT_ARP: {
                assert(req->data != NULL);
                int arp_status = _service_arp_request(req);
                assert(arp_status != -ENOMEM);
                if (arp_status == 0) {
                    while (_servq.complete(req) == -ENOBUFS)
                        ;
                }
                break;
            }
            case dpdk::wait_type::WAIT_LCORE_TERM:
                rte_free(_rx_table);
                while (_servq.complete(req) == -ENOBUFS)
                    ;
                // Return a positive value to indicate we should terminate
                return 1;
            default:
                UHD_LOG_ERROR(
                    "DPDK::IO_SERVICE", "Invalid reason associated with wait request");
                while (_servq.complete(req) == -ENOBUFS)
                    ;
                break;
        }
    }
    return 0;
}

void dpdk_io_service::_service_flow_open(dpdk::wait_req* req)
{
    auto flow_req_data = (struct dpdk_flow_data*)req->data;
    assert(flow_req_data);
    if (flow_req_data->is_recv) {
        // If RX, add to RX table. Currently, nothing to do for TX.
        struct dpdk::ipv4_5tuple ht_key = {.flow_type = dpdk::flow_type::FLOW_TYPE_UDP,
            .src_ip                                   = 0,
            .dst_ip   = flow_req_data->link->get_port()->get_ipv4(),
            .src_port = 0,
            .dst_port = flow_req_data->link->get_local_port()};
        // Check the UDP port isn't in use
        if (rte_hash_lookup(_rx_table, &ht_key) > 0) {
            req->retval = -EADDRINUSE;
            UHD_LOG_ERROR("DPDK::IO_SERVICE", "Cannot add to RX table");
            while (_servq.complete(req) == -ENOBUFS)
                ;
            return;
        }
        // Add xport list for this UDP port
        auto rx_entry = new std::list<dpdk_io_if*>();
        if (rte_hash_add_key_data(_rx_table, &ht_key, rx_entry)) {
            UHD_LOG_ERROR("DPDK::IO_SERVICE", "Could not add new RX list to table");
            delete rx_entry;
            req->retval = -ENOMEM;
            while (_servq.complete(req) == -ENOBUFS)
                ;
            return;
        }
    }
    while (_servq.complete(req) == -ENOBUFS)
        ;
}

void dpdk_io_service::_service_flow_close(dpdk::wait_req* req)
{
    auto flow_req_data = (struct dpdk_flow_data*)req->data;
    assert(flow_req_data);
    if (flow_req_data->is_recv) {
        // If RX, remove from RX table. Currently, nothing to do for TX.
        struct dpdk::ipv4_5tuple ht_key = {.flow_type = dpdk::flow_type::FLOW_TYPE_UDP,
            .src_ip                                   = 0,
            .dst_ip   = flow_req_data->link->get_port()->get_ipv4(),
            .src_port = 0,
            .dst_port = flow_req_data->link->get_local_port()};
        std::list<dpdk_io_if*>* xport_list;

        if (rte_hash_lookup_data(_rx_table, &ht_key, (void**)&xport_list) > 0) {
            UHD_ASSERT_THROW(xport_list->empty());
            delete xport_list;
            rte_hash_del_key(_rx_table, &ht_key);
            while (_servq.complete(req) == -ENOBUFS)
                ;
            return;
        }
    }
    while (_servq.complete(req) == -ENOBUFS)
        ;
}

void dpdk_io_service::_service_xport_connect(dpdk::wait_req* req)
{
    auto dpdk_io = static_cast<dpdk_io_if*>(req->data);
    UHD_ASSERT_THROW(dpdk_io);
    auto port = dpdk_io->link->get_port();
    if (dpdk_io->recv_cb) {
        // Add to RX table only if have a callback.
        struct dpdk::ipv4_5tuple ht_key = {.flow_type = dpdk::flow_type::FLOW_TYPE_UDP,
            .src_ip                                   = 0,
            .dst_ip                                   = port->get_ipv4(),
            .src_port                                 = 0,
            .dst_port                                 = dpdk_io->link->get_local_port()};
        void* hash_data;
        if (rte_hash_lookup_data(_rx_table, &ht_key, &hash_data) < 0) {
            req->retval = -ENOENT;
            UHD_LOG_ERROR("DPDK::IO_SERVICE", "Cannot add xport to RX table");
            while (_servq.complete(req) == -ENOBUFS)
                ;
            return;
        }
        // Add to xport list for this UDP port
        auto rx_entry = (std::list<dpdk_io_if*>*)(hash_data);
        rx_entry->push_back(dpdk_io);
    }
    if (dpdk_io->is_recv) {
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Servicing RX connect request...");
        // Add to xport list for this NIC port
        auto& xport_list = _recv_xport_map.at(port->get_port_id());
        xport_list.push_back((dpdk_recv_io*)dpdk_io->io_client);
    } else {
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Servicing TX connect request...");
        dpdk_send_io* send_io = static_cast<dpdk_send_io*>(dpdk_io->io_client);
        // Add to xport list for this NIC port
        auto& xport_list = _tx_queues.at(port->get_port_id());
        xport_list.push_back(send_io);
        for (size_t i = 0; i < send_io->_num_send_frames; i++) {
            auto buff_ptr =
                (dpdk::dpdk_frame_buff*)dpdk_io->link->get_send_buff(0).release();
            if (!buff_ptr) {
                UHD_LOG_ERROR("DPDK::IO_SERVICE",
                    "TX mempool out of memory. Please increase dpdk_num_mbufs.");
                break;
            }
            if (rte_ring_enqueue(send_io->_buffer_queue, buff_ptr)) {
                rte_pktmbuf_free(buff_ptr->get_pktmbuf());
                break;
            }
            send_io->_num_frames_in_use++;
        }
    }
    while (_servq.complete(req) == -ENOBUFS)
        ;
}

void dpdk_io_service::_service_xport_disconnect(dpdk::wait_req* req)
{
    auto dpdk_io = (struct dpdk_io_if*)req->data;
    assert(dpdk_io);
    auto port = dpdk_io->link->get_port();
    if (dpdk_io->recv_cb) {
        // Remove from RX table only if have a callback.
        struct dpdk::ipv4_5tuple ht_key = {.flow_type = dpdk::flow_type::FLOW_TYPE_UDP,
            .src_ip                                   = 0,
            .dst_ip                                   = port->get_ipv4(),
            .src_port                                 = 0,
            .dst_port                                 = dpdk_io->link->get_local_port()};
        void* hash_data;
        if (rte_hash_lookup_data(_rx_table, &ht_key, &hash_data) >= 0) {
            // Remove from xport list for this UDP port
            auto rx_entry = (std::list<dpdk_io_if*>*)(hash_data);
            rx_entry->remove(dpdk_io);
        } else {
            req->retval = -EINVAL;
            UHD_LOG_ERROR("DPDK::IO_SERVICE", "Cannot remove xport from RX table");
        }
    }
    if (dpdk_io->is_recv) {
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Servicing RX disconnect request...");
        dpdk_recv_io* recv_client = static_cast<dpdk_recv_io*>(dpdk_io->io_client);
        // Remove from xport list for this NIC port
        auto& xport_list = _recv_xport_map.at(port->get_port_id());
        xport_list.remove(recv_client);
        while (!rte_ring_empty(recv_client->_recv_queue)) {
            frame_buff* buff_ptr;
            rte_ring_dequeue(recv_client->_recv_queue, (void**)&buff_ptr);
            dpdk_io->link->release_recv_buff(frame_buff::uptr(buff_ptr));
        }
        while (!rte_ring_empty(recv_client->_release_queue)) {
            frame_buff* buff_ptr;
            rte_ring_dequeue(recv_client->_release_queue, (void**)&buff_ptr);
            dpdk_io->link->release_recv_buff(frame_buff::uptr(buff_ptr));
        }
    } else {
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Servicing TX disconnect request...");
        dpdk_send_io* send_client = static_cast<dpdk_send_io*>(dpdk_io->io_client);
        // Remove from xport list for this NIC port
        auto& xport_list = _tx_queues.at(port->get_port_id());
        xport_list.remove(send_client);
        while (!rte_ring_empty(send_client->_send_queue)) {
            frame_buff* buff_ptr;
            rte_ring_dequeue(send_client->_send_queue, (void**)&buff_ptr);
            dpdk_io->link->release_send_buff(frame_buff::uptr(buff_ptr));
        }
        while (!rte_ring_empty(send_client->_buffer_queue)) {
            frame_buff* buff_ptr;
            rte_ring_dequeue(send_client->_buffer_queue, (void**)&buff_ptr);
            dpdk_io->link->release_send_buff(frame_buff::uptr(buff_ptr));
        }
    }
    // Now remove the node if it's on the retry list
    if ((_retry_head == dpdk_io) && (dpdk_io->next == dpdk_io)) {
        _retry_head = NULL;
    } else if (_retry_head) {
        dpdk_io_if* node = _retry_head->next;
        while (node != _retry_head) {
            if (node == dpdk_io) {
                dpdk_io->prev->next = dpdk_io->next;
                dpdk_io->next->prev = dpdk_io->prev;
                break;
            }
            node = node->next;
        }
    }
    while (_servq.complete(req) == -ENOBUFS)
        ;
}

int dpdk_io_service::_service_arp_request(dpdk::wait_req* req)
{
    int status               = 0;
    auto arp_req_data        = (struct dpdk::arp_request*)req->data;
    dpdk::rte_ipv4_addr dst_addr = arp_req_data->tpa;
    auto ctx_sptr            = _ctx.lock();
    UHD_ASSERT_THROW(ctx_sptr);
    dpdk::dpdk_port* port = ctx_sptr->get_port(arp_req_data->port);
    UHD_LOG_TRACE("DPDK::IO_SERVICE",
        "ARP: Requesting address for " << dpdk::ipv4_num_to_str(dst_addr));

    rte_spinlock_lock(&port->_spinlock);
    struct dpdk::arp_entry* entry = NULL;
    if (port->_arp_table.count(dst_addr) == 0) {
        entry = (struct dpdk::arp_entry*)rte_zmalloc(NULL, sizeof(*entry), 0);
        if (!entry) {
            status = -ENOMEM;
            goto arp_end;
        }
        entry = new (entry) dpdk::arp_entry();
        entry->reqs.push_back(req);
        port->_arp_table[dst_addr] = entry;
        status                     = -EAGAIN;
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Address not in table. Sending ARP request.");
        _send_arp_request(port, 0, arp_req_data->tpa);
    } else {
        entry = port->_arp_table.at(dst_addr);
        if (rte_is_zero_ether_addr(&entry->mac_addr)) {
            UHD_LOG_TRACE("DPDK::IO_SERVICE",
                "ARP: Address in table, but not populated yet. Resending ARP request.");
            port->_arp_table.at(dst_addr)->reqs.push_back(req);
            status = -EAGAIN;
            _send_arp_request(port, 0, arp_req_data->tpa);
        } else {
            UHD_LOG_TRACE("DPDK::IO_SERVICE", "ARP: Address in table.");
            rte_ether_addr_copy(&entry->mac_addr, &arp_req_data->tha);
            status = 0;
        }
    }
arp_end:
    rte_spinlock_unlock(&port->_spinlock);
    return status;
}

int dpdk_io_service::_send_arp_request(
    dpdk::dpdk_port* port, dpdk::queue_id_t queue, dpdk::rte_ipv4_addr ip)
{
    struct rte_mbuf* mbuf;
    struct rte_ether_hdr* hdr;
    struct rte_arp_hdr* arp_frame;

    mbuf = rte_pktmbuf_alloc(port->get_tx_pktbuf_pool());
    if (unlikely(mbuf == NULL)) {
        UHD_LOG_WARNING(
            "DPDK::IO_SERVICE", "Could not allocate packet buffer for ARP request");
        return -ENOMEM;
    }

    hdr       = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
    arp_frame = (struct rte_arp_hdr*)&hdr[1];

    memset(hdr->d_addr.addr_bytes, 0xFF, RTE_ETHER_ADDR_LEN);
    hdr->s_addr     = port->get_mac_addr();
    hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP);

    arp_frame->arp_hardware          = rte_cpu_to_be_16(RTE_ARP_HRD_ETHER);
    arp_frame->arp_protocol          = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    arp_frame->arp_hlen          = 6;
    arp_frame->arp_plen          = 4;
    arp_frame->arp_opcode           = rte_cpu_to_be_16(RTE_ARP_OP_REQUEST);
    arp_frame->arp_data.arp_sha = port->get_mac_addr();
    arp_frame->arp_data.arp_sip = port->get_ipv4();
    memset(arp_frame->arp_data.arp_tha.addr_bytes, 0x00, RTE_ETHER_ADDR_LEN);
    arp_frame->arp_data.arp_tip = ip;

    mbuf->pkt_len  = 42;
    mbuf->data_len = 42;

    if (rte_eth_tx_burst(port->get_port_id(), queue, &mbuf, 1) != 1) {
        UHD_LOG_WARNING("DPDK::IO_SERVICE", "ARP request not sent: Descriptor ring full");
        rte_pktmbuf_free(mbuf);
        return -EAGAIN;
    }
    return 0;
}

/* Do a burst of RX on port */
int dpdk_io_service::_rx_burst(dpdk::dpdk_port* port, dpdk::queue_id_t queue)
{
    struct rte_ether_hdr* hdr;
    char* l2_data;
    struct rte_mbuf* bufs[RX_BURST_SIZE];
    const uint16_t num_rx =
        rte_eth_rx_burst(port->get_port_id(), queue, bufs, RX_BURST_SIZE);
    if (unlikely(num_rx == 0)) {
        return 0;
    }

    for (int buf = 0; buf < num_rx; buf++) {
        uint64_t ol_flags = bufs[buf]->ol_flags;
        hdr               = rte_pktmbuf_mtod(bufs[buf], struct rte_ether_hdr*);
        l2_data           = (char*)&hdr[1];
        switch (rte_be_to_cpu_16(hdr->ether_type)) {
            case RTE_ETHER_TYPE_ARP:
                _process_arp(port, queue, (struct rte_arp_hdr*)l2_data);
                rte_pktmbuf_free(bufs[buf]);
                break;
            case RTE_ETHER_TYPE_IPV4:
                if ((ol_flags & PKT_RX_IP_CKSUM_MASK) == PKT_RX_IP_CKSUM_BAD) {
                    UHD_LOG_WARNING("DPDK::IO_SERVICE", "RX packet has bad IP cksum");
                } else if ((ol_flags & PKT_RX_IP_CKSUM_MASK) == PKT_RX_IP_CKSUM_NONE) {
                    UHD_LOG_WARNING("DPDK::IO_SERVICE", "RX packet missing IP cksum");
                } else {
                    _process_ipv4(port, bufs[buf], (struct rte_ipv4_hdr*)l2_data);
                }
                break;
            default:
                rte_pktmbuf_free(bufs[buf]);
                break;
        }
    }
    return num_rx;
}

int dpdk_io_service::_process_arp(
    dpdk::dpdk_port* port, dpdk::queue_id_t queue_id, struct rte_arp_hdr* arp_frame)
{
    uint32_t dest_ip            = arp_frame->arp_data.arp_sip;
    struct rte_ether_addr dest_addr = arp_frame->arp_data.arp_sha;
    UHD_LOG_TRACE("DPDK::IO_SERVICE",
        "Processing ARP packet: " << dpdk::ipv4_num_to_str(dest_ip) << " -> "
                                  << dpdk::eth_addr_to_string(dest_addr));
    /* Add entry to ARP table */
    rte_spinlock_lock(&port->_spinlock);
    struct dpdk::arp_entry* entry = NULL;
    if (port->_arp_table.count(dest_ip) == 0) {
        entry = (struct dpdk::arp_entry*)rte_zmalloc(NULL, sizeof(*entry), 0);
        if (!entry) {
            return -ENOMEM;
        }
        entry = new (entry) dpdk::arp_entry();
        rte_ether_addr_copy(&dest_addr, &entry->mac_addr);
        port->_arp_table[dest_ip] = entry;
    } else {
        entry = port->_arp_table.at(dest_ip);
        rte_ether_addr_copy(&dest_addr, &entry->mac_addr);
        for (auto req : entry->reqs) {
            auto arp_data = (struct dpdk::arp_request*)req->data;
            rte_ether_addr_copy(&dest_addr, &arp_data->tha);
            while (_servq.complete(req) == -ENOBUFS)
                ;
        }
        entry->reqs.clear();
    }
    rte_spinlock_unlock(&port->_spinlock);

    /* Respond if this was an ARP request */
    if (arp_frame->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REQUEST)
        && arp_frame->arp_data.arp_tip == port->get_ipv4()) {
        UHD_LOG_TRACE("DPDK::IO_SERVICE", "Sending ARP reply.");
        port->_arp_reply(queue_id, arp_frame);
    }

    return 0;
}

int dpdk_io_service::_process_ipv4(
    dpdk::dpdk_port* port, struct rte_mbuf* mbuf, struct rte_ipv4_hdr* pkt)
{
    bool bcast = port->dst_is_broadcast(pkt->dst_addr);
    if (pkt->dst_addr != port->get_ipv4() && !bcast) {
        rte_pktmbuf_free(mbuf);
        return -ENODEV;
    }
    if (pkt->next_proto_id == IPPROTO_UDP) {
        return _process_udp(port, mbuf, (struct rte_udp_hdr*)&pkt[1], bcast);
    }
    rte_pktmbuf_free(mbuf);
    return -EINVAL;
}


int dpdk_io_service::_process_udp(
    dpdk::dpdk_port* port, struct rte_mbuf* mbuf, struct rte_udp_hdr* pkt, bool /*bcast*/)
{
    // Get the link
    struct dpdk::ipv4_5tuple ht_key = {.flow_type = dpdk::flow_type::FLOW_TYPE_UDP,
        .src_ip                                   = 0,
        .dst_ip                                   = port->get_ipv4(),
        .src_port                                 = 0,
        .dst_port                                 = pkt->dst_port};
    void* hash_data;
    if (rte_hash_lookup_data(_rx_table, &ht_key, &hash_data) < 0) {
        UHD_LOG_WARNING("DPDK::IO_SERVICE", "Dropping packet: No link entry in rx table");
        rte_pktmbuf_free(mbuf);
        return -ENOENT;
    }
    // Get xport list for this UDP port
    auto rx_entry = (std::list<dpdk_io_if*>*)(hash_data);
    if (rx_entry->empty()) {
        UHD_LOG_WARNING("DPDK::IO_SERVICE", "Dropping packet: No xports for link");
        rte_pktmbuf_free(mbuf);
        return -ENOENT;
    }
    // Turn rte_mbuf -> dpdk_frame_buff
    auto link = rx_entry->front()->link;
    link->enqueue_recv_mbuf(mbuf);
    auto buff       = link->get_recv_buff(0);
    bool rcvr_found = false;
    for (auto client_if : *rx_entry) {
        // Check all the muxed receivers...
        if (client_if->recv_cb(buff, link, link)) {
            rcvr_found = true;
            if (buff) {
                assert(client_if->is_recv);
                auto recv_io  = (dpdk_recv_io*)client_if->io_client;
                auto buff_ptr = (dpdk::dpdk_frame_buff*)buff.release();
                if (rte_ring_enqueue(recv_io->_recv_queue, buff_ptr)) {
                    rte_pktmbuf_free(buff_ptr->get_pktmbuf());
                    UHD_LOG_WARNING(
                        "DPDK::IO_SERVICE", "Dropping packet: No space in recv queue");
                } else {
                    recv_io->_num_frames_in_use++;
                    assert(recv_io->_num_frames_in_use <= recv_io->_num_recv_frames);
                    _wake_client(client_if);
                }
            }
            break;
        }
    }
    if (!rcvr_found) {
        UHD_LOG_WARNING("DPDK::IO_SERVICE", "Dropping packet: No receiver xport found");
        // Release the buffer if no receiver found
        link->release_recv_buff(std::move(buff));
        return -ENOENT;
    }
    return 0;
}

/* Do a burst of TX on port's tx queues */
int dpdk_io_service::_tx_burst(dpdk::dpdk_port* port)
{
    unsigned int total_tx = 0;
    auto& queues          = _tx_queues.at(port->get_port_id());

    for (auto& send_io : queues) {
        unsigned int num_tx   = rte_ring_count(send_io->_send_queue);
        num_tx                = (num_tx < TX_BURST_SIZE) ? num_tx : TX_BURST_SIZE;
        bool replaced_buffers = false;
        for (unsigned int i = 0; i < num_tx; i++) {
            size_t frame_size = send_io->_dpdk_io_if.link->get_send_frame_size();
            if (send_io->_fc_cb && !send_io->_fc_cb(frame_size)) {
                break;
            }
            dpdk::dpdk_frame_buff* buff_ptr;
            int status = rte_ring_dequeue(send_io->_send_queue, (void**)&buff_ptr);
            if (status) {
                UHD_LOG_ERROR("DPDK::IO_SERVICE", "TX Q Count doesn't match actual");
                break;
            }
            send_io->_send_cb(frame_buff::uptr(buff_ptr), send_io->_dpdk_io_if.link);
            // Attempt to replace buffer
            buff_ptr = (dpdk::dpdk_frame_buff*)send_io->_dpdk_io_if.link->get_send_buff(0)
                           .release();
            if (!buff_ptr) {
                UHD_LOG_ERROR("DPDK::IO_SERVICE",
                    "TX mempool out of memory. Please increase dpdk_num_mbufs.");
                send_io->_num_frames_in_use--;
            } else if (rte_ring_enqueue(send_io->_buffer_queue, buff_ptr)) {
                rte_pktmbuf_free(buff_ptr->get_pktmbuf());
                send_io->_num_frames_in_use--;
            } else {
                replaced_buffers = true;
            }
        }
        if (replaced_buffers) {
            _wake_client(&send_io->_dpdk_io_if);
        }
        total_tx += num_tx;
    }

    return total_tx;
}

int dpdk_io_service::_rx_release(dpdk::dpdk_port* port)
{
    unsigned int total_bufs = 0;
    auto& queues            = _recv_xport_map.at(port->get_port_id());

    for (auto& recv_io : queues) {
        unsigned int num_buf = rte_ring_count(recv_io->_release_queue);
        num_buf              = (num_buf < RX_BURST_SIZE) ? num_buf : RX_BURST_SIZE;
        for (unsigned int i = 0; i < num_buf; i++) {
            dpdk::dpdk_frame_buff* buff_ptr;
            int status = rte_ring_dequeue(recv_io->_release_queue, (void**)&buff_ptr);
            if (status) {
                UHD_LOG_ERROR("DPDK::IO_SERVICE", "RX Q Count doesn't match actual");
                break;
            }
            recv_io->_fc_cb(frame_buff::uptr(buff_ptr),
                recv_io->_dpdk_io_if.link,
                recv_io->_dpdk_io_if.link);
            recv_io->_num_frames_in_use--;
        }
        total_bufs += num_buf;
    }

    return total_bufs;
}

uint16_t dpdk_io_service::_get_unique_client_id()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_client_id_set.size() >= MAX_CLIENTS) {
        UHD_LOG_ERROR("DPDK::IO_SERVICE", "Exceeded maximum number of clients");
        throw uhd::runtime_error("DPDK::IO_SERVICE: Exceeded maximum number of clients");
    }

    uint16_t id = _next_client_id++;
    while (_client_id_set.count(id)) {
        id = _next_client_id++;
    }
    _client_id_set.insert(id);
    return id;
}

void dpdk_io_service::_wake_client(dpdk_io_if* dpdk_io)
{
    dpdk::wait_req* req;
    if (dpdk_io->is_recv) {
        auto recv_io = static_cast<dpdk_recv_io*>(dpdk_io->io_client);
        req          = recv_io->_waiter;
    } else {
        auto send_io = static_cast<dpdk_send_io*>(dpdk_io->io_client);
        req          = send_io->_waiter;
    }
    bool stat = req->mutex.try_lock();
    if (stat) {
        bool active_req = !req->complete;
        if (dpdk_io->next) {
            // On the list: Take it off
            if (dpdk_io->next == dpdk_io) {
                // Only node on the list
                _retry_head = NULL;
            } else {
                // Other nodes are on the list
                if (_retry_head == dpdk_io) {
                    // Move list head to next
                    _retry_head = dpdk_io->next;
                }
                dpdk_io->next->prev = dpdk_io->prev;
                dpdk_io->prev->next = dpdk_io->next;
            }
            dpdk_io->next = NULL;
            dpdk_io->prev = NULL;
        }
        if (active_req) {
            req->complete = true;
            req->cond.notify_one();
        }
        req->mutex.unlock();
        if (active_req) {
            wait_req_put(req);
        }
    } else {
        // Put on the retry list, if it isn't already
        if (!dpdk_io->next) {
            if (_retry_head) {
                dpdk_io->next           = _retry_head;
                dpdk_io->prev           = _retry_head->prev;
                _retry_head->prev->next = dpdk_io;
                _retry_head->prev       = dpdk_io;
            } else {
                _retry_head   = dpdk_io;
                dpdk_io->next = dpdk_io;
                dpdk_io->prev = dpdk_io;
            }
        }
    }
}
