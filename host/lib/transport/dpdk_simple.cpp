//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/frame_buff.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/dpdk/udp.hpp>
#include <uhdlib/transport/dpdk_io_service.hpp>
#include <uhdlib/transport/dpdk_io_service_client.hpp>
#include <uhdlib/transport/dpdk_simple.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/transport/udp_dpdk_link.hpp>
#include <arpa/inet.h>


namespace uhd { namespace transport {

namespace {
constexpr double SEND_TIMEOUT_MS = 500; // seconds
}

class dpdk_simple_impl : public dpdk_simple
{
public:
    dpdk_simple_impl(const std::string& addr, const std::string& port)
    {
        link_params_t link_params = _get_default_link_params();
        _link = uhd::transport::udp_dpdk_link::make(addr, port, link_params);
        UHD_LOG_TRACE("DPDK::SIMPLE",
            "Creating simple UDP object for " << addr << ":" << port
                                              << ", DPDK port index "
                                              << _link->get_port()->get_port_id());
        // The context must be initialized, or we'd never get here
        auto ctx = uhd::transport::dpdk::dpdk_ctx::get();
        UHD_ASSERT_THROW(ctx->is_init_done());

        // Init I/O service
        _port_id    = _link->get_port()->get_port_id();
        _io_service = ctx->get_io_service(_port_id);
        // This is normally done by the I/O service manager, but with DPDK, this
        // is all it does so we skip that step
        UHD_LOG_TRACE("DPDK::SIMPLE", "Attaching link to I/O service...");
        _io_service->attach_recv_link(_link);
        _io_service->attach_send_link(_link);

        auto recv_cb = [this](buff_t::uptr& buff, recv_link_if*, send_link_if*) {
            return this->_recv_callback(buff);
        };

        auto fc_cb = [this](buff_t::uptr buff, recv_link_if*, send_link_if*) {
            this->_recv_fc_callback(std::move(buff));
        };

        _recv_io = _io_service->make_recv_client(_link,
            link_params.num_recv_frames,
            recv_cb,
            nullptr, // No send/fc link
            0, // No send frames
            fc_cb);

        auto send_cb = [this](buff_t::uptr buff, transport::send_link_if*) {
            this->_send_callback(std::move(buff));
        };
        _send_io = _io_service->make_send_client(_link,
            link_params.num_send_frames,
            send_cb,
            nullptr, // no FC link
            0,
            nullptr, // No receive callback necessary
            [](const size_t) { return true; } // We can always send
        );
        UHD_LOG_TRACE("DPDK::SIMPLE", "Constructor complete");
    }

    ~dpdk_simple_impl(void) override
    {
        UHD_LOG_TRACE("DPDK::SIMPLE",
            "~dpdk_simple_impl(), DPDK port index " << _link->get_port()->get_port_id());
        // Disconnect the clients from the I/O service
        _send_io.reset();
        _recv_io.reset();
        // Disconnect the link from the I/O service
        _io_service->detach_recv_link(_link);
        _io_service->detach_send_link(_link);
    }

    /*! Send and release outstanding buffer
     *
     * \param length bytes of data to send
     * \return number of bytes sent (releases buffer if sent)
     */
    size_t send(const boost::asio::const_buffer& user_buff) override
    {
        // Extract buff and sanity check
        const size_t nbytes = boost::asio::buffer_size(user_buff);
        UHD_ASSERT_THROW(nbytes <= _link->get_send_frame_size())
        const uint8_t* user_data = boost::asio::buffer_cast<const uint8_t*>(user_buff);

        // Get send buff
        auto buff = _send_io->get_send_buff(SEND_TIMEOUT_MS);
        UHD_ASSERT_THROW(buff);
        buff->set_packet_size(nbytes);
        std::memcpy(buff->data(), user_data, nbytes);

        // Release send buff (send the packet)
        _send_io->release_send_buff(std::move(buff));
        return nbytes;
    }

    /*! Receive a single packet.
     *
     * Buffer provided by transport (must be freed before next operation).
     *
     * \param buf a pointer to place to write buffer location
     * \param timeout the timeout in seconds
     * \return the number of bytes received or zero on timeout
     */
    size_t recv(const boost::asio::mutable_buffer& user_buff, double timeout) override
    {
        size_t user_buff_size = boost::asio::buffer_size(user_buff);
        uint8_t* user_data    = boost::asio::buffer_cast<uint8_t*>(user_buff);

        auto buff = _recv_io->get_recv_buff(static_cast<int32_t>(timeout * 1e3));
        if (!buff) {
            return 0;
        }

        // Extract the sender's address. This is only possible because we know
        // the memory layout of the buff
        struct rte_udp_hdr* rte_udp_hdr_end = (struct rte_udp_hdr*)buff->data();
        struct rte_ipv4_hdr* ip_hdr_end = (struct rte_ipv4_hdr*)(&rte_udp_hdr_end[-1]);
        struct rte_ipv4_hdr* ip_hdr     = (struct rte_ipv4_hdr*)(&ip_hdr_end[-1]);
        _last_recv_addr             = ip_hdr->src_addr;

        // Extract the buffer data
        const size_t copy_len = std::min(user_buff_size, buff->packet_size());
        if (copy_len < buff->packet_size()) {
            UHD_LOG_WARNING("DPDK", "Truncating recv packet");
        }
        std::memcpy(user_data, buff->data(), copy_len);

        // Housekeeping
        _recv_io->release_recv_buff(std::move(buff));
        return copy_len;
    }

    std::string get_recv_addr(void) override
    {
        return dpdk::ipv4_num_to_str(_last_recv_addr);
    }

    std::string get_send_addr(void) override
    {
        return dpdk::ipv4_num_to_str(_link->get_remote_ipv4());
    }

private:
    using buff_t = frame_buff;

    link_params_t _get_default_link_params()
    {
        link_params_t link_params;
        link_params.recv_frame_size = 8000;
        link_params.send_frame_size = 8000;
        link_params.num_recv_frames = 1;
        link_params.num_send_frames = 1;
        link_params.recv_buff_size  = 8000;
        link_params.send_buff_size  = 8000;
        return link_params;
    }

    void _send_callback(buff_t::uptr buff)
    {
        _link->release_send_buff(std::move(buff));
    }

    bool _recv_callback(buff_t::uptr&)
    {
        // Queue it up
        return true;
    }

    void _recv_fc_callback(buff_t::uptr buff)
    {
        _link->release_recv_buff(std::move(buff));
    }

    /*** Attributes **********************************************************/
    unsigned int _port_id;
    uint32_t _last_recv_addr;

    udp_dpdk_link::sptr _link;

    dpdk_io_service::sptr _io_service;

    send_io_if::sptr _send_io;

    recv_io_if::sptr _recv_io;
};

dpdk_simple::~dpdk_simple(void) {}

/***********************************************************************
 * DPDK simple transport public make functions
 **********************************************************************/
udp_simple::sptr dpdk_simple::make_connected(
    const std::string& addr, const std::string& port)
{
    return udp_simple::sptr(new dpdk_simple_impl(addr, port));
}

// For DPDK, this is not special and the same as make_connected
udp_simple::sptr dpdk_simple::make_broadcast(
    const std::string& addr, const std::string& port)
{
    return udp_simple::sptr(new dpdk_simple_impl(addr, port));
}
}} // namespace uhd::transport
