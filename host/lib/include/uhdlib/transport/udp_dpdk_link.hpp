//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <uhdlib/transport/links.hpp>
#include <rte_udp.h>
#include <cassert>
#include <string>
#include <vector>

namespace uhd { namespace transport {

/*!
 * A zero copy transport interface to the dpdk DMA library.
 */
class udp_dpdk_link : public virtual recv_link_if, public virtual send_link_if
{
public:
    using sptr = std::shared_ptr<udp_dpdk_link>;

    udp_dpdk_link(const dpdk::port_id_t port_id,
        const std::string& remote_addr,
        const std::string& remote_port,
        const std::string& local_port,
        const link_params_t& params);

    ~udp_dpdk_link();

    /*!
     * Make a new dpdk link. Get port ID from routing table.
     *
     * \param remote_addr Remote IP address
     * \param remote_port Remote UDP port
     * \param params Values for frame sizes, num frames, and buffer sizes
     * \return a shared_ptr to a new udp dpdk link
     */
    static sptr make(const std::string& remote_addr,
        const std::string& remote_port,
        const link_params_t& params);

    /*!
     * Make a new dpdk link. User specifies DPDK port ID directly.
     *
     * \param port_id DPDK port ID to use for communication
     * \param remote_addr Remote IP address
     * \param remote_port Remote UDP port
     * \param local_port Local UDP port
     * \param params Values for frame sizes, num frames, and buffer sizes
     * \return a shared_ptr to a new udp dpdk link
     */
    static sptr make(const dpdk::port_id_t port_id,
        const std::string& remote_addr,
        const std::string& remote_port,
        const std::string& local_port,
        const link_params_t& params);

    /*!
     * Get the associated dpdk_port
     *
     * \return a pointer to the dpdk_port used by this link
     */
    inline dpdk::dpdk_port* get_port()
    {
        return _port;
    }

    /*!
     * Get the DMA queue associated with this link
     *
     * \return the queue ID for this link's DMA queue
     */
    inline dpdk::queue_id_t get_queue_id()
    {
        return _queue;
    }

    /*!
     * Get the local UDP port used by this link
     *
     * \return the local UDP port, in network order
     */
    inline uint16_t get_local_port()
    {
        return _local_port;
    }

    /*!
     * Get the remote UDP port used by this link
     *
     * \return the remote UDP port, in network order
     */
    inline uint16_t get_remote_port()
    {
        return _remote_port;
    }

    /*!
     * Get the remote IPv4 address used by this link
     *
     * \return the remote IPv4 address, in network order
     */
    inline uint32_t get_remote_ipv4()
    {
        return _remote_ipv4;
    }

    /*!
     * Set the remote host's MAC address
     * This MAC address must be filled in for the remote IPv4 address before
     * the link can reach its destination.
     *
     * \param mac the remote host's MAC address
     */
    inline void set_remote_mac(struct rte_ether_addr& mac)
    {
        rte_ether_addr_copy(&mac, &_remote_mac);
    }

    /*!
     * Get the remote host's MAC address
     *
     * \param mac Where to write the MAC address
     */
    inline void get_remote_mac(struct rte_ether_addr& dst)
    {
        rte_ether_addr_copy(&_remote_mac, &dst);
    }

    /*!
     * Get the number of frame buffers that can be queued by this link.
     */
    size_t get_num_send_frames() const
    {
        return _num_send_frames;
    }

    /*!
     * Get the maximum capacity of a frame buffer.
     */
    size_t get_send_frame_size() const
    {
        return _send_frame_size;
    }

    /*!
     * Get the physical adapter ID used for this link
     */
    inline adapter_id_t get_send_adapter_id() const
    {
        return _adapter_id;
    }

    /*!
     * Get the number of frame buffers that can be queued by this link.
     */
    size_t get_num_recv_frames() const
    {
        return _num_recv_frames;
    }

    /*!
     * Get the maximum capacity of a frame buffer.
     */
    size_t get_recv_frame_size() const
    {
        return _recv_frame_size;
    }

    /*!
     * Get the physical adapter ID used for this link
     */
    inline adapter_id_t get_recv_adapter_id() const
    {
        return _adapter_id;
    }

    /*!
     * Enqueue a received mbuf, which can be pulled via get_recv_buff()
     */
    void enqueue_recv_mbuf(struct rte_mbuf* mbuf);

    /*!
     * Receive a packet and return a frame buffer containing the packet data.
     * The timeout argument is ignored.
     *
     * Received buffers are pulled from the frame buffer list. No buffers can
     * be retrieved unless the corresponding rte_mbufs were placed in the list
     * via the enqueue_recv_mbuf() method.
     *
     * \return a frame buffer, or null uptr if timeout occurs
     */
    frame_buff::uptr get_recv_buff(int32_t /*timeout_ms*/);

    /*!
     * Release a frame buffer, allowing the link driver to reuse it.
     *
     * \param buffer frame buffer to release for reuse by the link
     */
    void release_recv_buff(frame_buff::uptr buff);

    /*!
     * Get an empty frame buffer in which to write packet contents.
     *
     * \param timeout_ms a positive timeout value specifies the maximum number
                         of ms to wait, a negative value specifies to block
                         until successful, and a value of 0 specifies no wait.
     * \return a frame buffer, or null uptr if timeout occurs
     */
    frame_buff::uptr get_send_buff(int32_t /*timeout_ms*/);

    /*!
     * Send a packet with the contents of the frame buffer and release the
     * buffer, allowing the link driver to reuse it. If the size of the frame
     * buffer is 0, the buffer is released with no packet being sent.
     *
     * Note that this function will only fill in the L2 header and send the
     * mbuf. The L3 and L4 headers, in addition to the lengths in the rte_mbuf
     * fields, must be set in the I/O service.
     *
     * \param buffer frame buffer containing packet data
     *
     * Throws an exception if an I/O error occurs while sending
     */
    void release_send_buff(frame_buff::uptr buff);

private:
    //! A reference to the DPDK context
    dpdk::dpdk_ctx::sptr _ctx;
    //! The DPDK NIC port used by this link
    dpdk::dpdk_port* _port;
    //! Local UDP port, in network order
    uint16_t _local_port;
    //! Remote UDP port, in network order
    uint16_t _remote_port;
    //! Remote IPv4 address, in network order
    uint32_t _remote_ipv4;
    //! Remote host's MAC address
    struct rte_ether_addr _remote_mac;
    //! Number of recv frames is not validated
    size_t _num_recv_frames;
    //! Maximum bytes of UDP payload data in recv frame
    size_t _recv_frame_size;
    //! Number of send frames is not validated
    size_t _num_send_frames;
    //! Maximum bytes of UDP payload data in send frame
    size_t _send_frame_size;
    //! Registered adapter ID for this link's DPDK NIC port
    adapter_id_t _adapter_id;
    //! The RX frame buff list head
    dpdk::dpdk_frame_buff* _recv_buff_head = nullptr;
    // TODO: Implement ability to use multiple queues
    dpdk::queue_id_t _queue = 0;
};

}} // namespace uhd::transport
