//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk/service_queue.hpp>
#include <uhdlib/transport/io_service.hpp>
#include <rte_arp.h>
#include <rte_hash.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_udp.h>
#include <vector>

namespace uhd { namespace transport {

class dpdk_send_io;
class dpdk_recv_io;
struct dpdk_io_if;

class dpdk_io_service : public virtual io_service,
                        public std::enable_shared_from_this<dpdk_io_service>
{
public:
    using sptr = std::shared_ptr<dpdk_io_service>;

    static sptr make(
        unsigned int lcore_id, std::vector<dpdk::dpdk_port*> ports, size_t servq_depth);

    ~dpdk_io_service();

    // Add entry to RX flow table
    // This yields a link, which is then used for attaching to a buffer
    // We yank from the link immediately following, then process at the transport level
    // (so two tables here, one for transports, one for links)
    void attach_recv_link(recv_link_if::sptr link);

    // Create object to hold set of queues, to go in TX table
    void attach_send_link(send_link_if::sptr link);

    void detach_recv_link(recv_link_if::sptr link);

    void detach_send_link(send_link_if::sptr link);

    recv_io_if::sptr make_recv_client(recv_link_if::sptr data_link,
        size_t num_recv_frames,
        recv_callback_t cb,
        send_link_if::sptr fc_link,
        size_t num_send_frames,
        recv_io_if::fc_callback_t fc_cb);

    send_io_if::sptr make_send_client(send_link_if::sptr send_link,
        size_t num_send_frames,
        send_io_if::send_callback_t send_cb,
        recv_link_if::sptr recv_link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        send_io_if::fc_callback_t fc_cb);


private:
    friend class dpdk_recv_io;
    friend class dpdk_send_io;

    dpdk_io_service(
        unsigned int lcore_id, std::vector<dpdk::dpdk_port*> ports, size_t servq_depth);
    dpdk_io_service(const dpdk_io_service&) = delete;

    /*!
     * I/O worker function to be passed to the DPDK lcore
     *
     * The argument must be a pointer to  *this* dpdk_io_service
     *
     * \param arg a pointer to this dpdk_io_service
     * \return 0 for normal termination, else nonzero
     */
    static int _io_worker(void* arg);

    /*!
     * Helper function for I/O thread to process requests on its service queue
     */
    int _service_requests();

    /*!
     * Helper function for I/O thread to service a WAIT_FLOW_OPEN request
     *
     * \param req The requester's wait_req object
     */
    void _service_flow_open(dpdk::wait_req* req);

    /*!
     * Helper function for I/O thread to service a WAIT_FLOW_CLOSE request
     *
     * \param req The requester's wait_req object
     */
    void _service_flow_close(dpdk::wait_req* req);

    /*!
     * Helper function for I/O thread to service a WAIT_XPORT_CONNECT request
     *
     * \param req The requester's wait_req object
     */
    void _service_xport_connect(dpdk::wait_req* req);

    /*!
     * Helper function for I/O thread to service a WAIT_XPORT_DISCONNECT request
     *
     * \param req The requester's wait_req object
     */
    void _service_xport_disconnect(dpdk::wait_req* req);

    /*!
     * Get Ethernet MAC address for the given IPv4 address, and wake the
     * requester when finished.
     * This may only be called by an I/O service, on behalf of a requester's
     * WAIT_ARP request.
     *
     * \param req The requester's wait_req object
     * \return 0 if address was written, -EAGAIN if request was queued for
     *         later completion, -ENOMEM if ran out of memory to complete
     *         request
     */
    int _service_arp_request(dpdk::wait_req* req);

    /*!
     * Helper function for I/O thread to do a burst of packet retrieval and
     * processing on an RX queue
     *
     * \param port the DPDK NIC port used for RX
     * \param queue the DMA queue on the port to recv from
     */
    int _rx_burst(dpdk::dpdk_port* port, dpdk::queue_id_t queue);

    /*!
     * Helper function for I/O thread to do a burst of packet transmission on a
     * TX queue
     *
     * \param port the DPDK NIC port used for TX
     * \return number of buffers transmitted
     */
    int _tx_burst(dpdk::dpdk_port* port);

    /*!
     * Helper function for I/O thread to release a burst of buffers from an RX
     * release queue
     *
     * \param port the DPDK NIC port used for RX
     * \return number of buffers released
     */
    int _rx_release(dpdk::dpdk_port* port);

    /*!
     * Helper function for I/O thread to do send an ARP request
     *
     * \param port the DPDK NIC port to send the ARP request through
     * \param queue the DMA queue on the port to send to
     * \param ip the IPv4 address for which the caller is seeking a MAC address
     */
    int _send_arp_request(
        dpdk::dpdk_port* port, dpdk::queue_id_t queue, dpdk::rte_ipv4_addr ip);

    /*!
     * Helper function for I/O thread to process an ARP request/reply
     *
     * \param port the DPDK NIC port to send any ARP replies from
     * \param queue the DMA queue on the port to send ARP replies to
     * \param arp_frame a pointer to the ARP frame
     */
    int _process_arp(
        dpdk::dpdk_port* port, dpdk::queue_id_t queue_id, struct rte_arp_hdr* arp_frame);

    /*!
     * Helper function for I/O thread to process an IPv4 packet
     *
     * \param port the DPDK NIC port to send any ARP replies from
     * \param mbuf a pointer to the packet buffer container
     * \param pkt a pointer to the IPv4 header of the packet
     */
    int _process_ipv4(dpdk::dpdk_port* port, struct rte_mbuf* mbuf, struct rte_ipv4_hdr* pkt);

    /*!
     * Helper function for I/O thread to process an IPv4 packet
     *
     * \param port the DPDK NIC port to send any ARP replies from
     * \param mbuf a pointer to the packet buffer container
     * \param pkt a pointer to the UDP header of the packet
     * \param bcast whether this packet was destined for the port's broadcast
     *              IPv4 address
     */
    int _process_udp(
        dpdk::dpdk_port* port, struct rte_mbuf* mbuf, struct rte_udp_hdr* pkt, bool bcast);

    /*!
     * Helper function to get a unique client ID
     *
     * \return a unique client ID
     */
    uint16_t _get_unique_client_id();

    /*!
     * Attempt to wake client
     */
    void _wake_client(dpdk_io_if* dpdk_io);

    //! The reference to the DPDK context
    std::weak_ptr<dpdk::dpdk_ctx> _ctx;
    //! The lcore running this dpdk_io_service's work routine
    unsigned int _lcore_id;
    //! The NIC ports served by this dpdk_io_service
    std::vector<dpdk::dpdk_port*> _ports;
    //! The set of TX queues associated with a given port
    std::unordered_map<dpdk::port_id_t, std::list<dpdk_send_io*>> _tx_queues;
    //! The list of recv_io for each port
    std::unordered_map<dpdk::port_id_t, std::list<dpdk_recv_io*>> _recv_xport_map;
    //! The RX table, which provides lists of dpdk_recv_io for an IPv4 tuple
    struct rte_hash* _rx_table;
    //! Service queue for clients to make requests
    dpdk::service_queue _servq;
    //! Retry list for waking clients
    dpdk_io_if* _retry_head = NULL;

    //! Mutex to protect below data structures
    std::mutex _mutex;
    //! The recv links attached to this I/O service (managed client side)
    std::list<recv_link_if::sptr> _recv_links;
    //! The send links attached to this I/O service (managed client side)
    std::list<send_link_if::sptr> _send_links;
    //! Set of IDs for new clients
    std::set<uint16_t> _client_id_set;
    //! Next ID to try
    uint16_t _next_client_id;

    static constexpr int MAX_PENDING_SERVICE_REQS = 32;
    static constexpr int MAX_FLOWS                = 128;
    static constexpr int MAX_CLIENTS              = 2048;
    static constexpr int RX_BURST_SIZE            = 16;
    static constexpr int TX_BURST_SIZE            = 16;
};

}} // namespace uhd::transport
