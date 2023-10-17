//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/transport/adapter_id.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/inline_io_service.hpp>
#include <uhdlib/transport/offload_io_service.hpp>
#ifdef HAVE_DPDK
#    include <uhdlib/usrp/common/dpdk_io_service_mgr.hpp>
#endif
#include <uhdlib/usrp/common/io_service_mgr.hpp>
#include <uhdlib/usrp/constrained_device_args.hpp>
#include <map>
#include <vector>

using namespace uhd;
using namespace uhd::transport;

static const std::string LOG_ID = "IO_SRV";

namespace uhd { namespace usrp {

/* This file defines an I/O service manager implementation, io_service_mgr_impl.
 * Its implementation is divided into three other classes, inline_io_service_mgr,
 * blocking_io_service_mgr, and polling_io_service_mgr. The io_service_mgr_impl
 * object selects which one to invoke based on the provided stream args.
 */

/* Inline I/O service manager
 *
 * I/O service manager for inline I/O services. Creates a new inline_io_service
 * for every new pair of links, unless they are already attached to an I/O
 * service (muxed links).
 */
class inline_io_service_mgr
{
public:
    io_service::sptr connect_links(
        recv_link_if::sptr recv_link, send_link_if::sptr send_link);

    void disconnect_links(recv_link_if::sptr recv_link, send_link_if::sptr send_link);

private:
    struct link_info_t
    {
        io_service::sptr io_srv;
        size_t mux_ref_count;
    };

    using link_pair_t = std::pair<recv_link_if::sptr, send_link_if::sptr>;
    std::map<link_pair_t, link_info_t> _link_info_map;
};

io_service::sptr inline_io_service_mgr::connect_links(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link)
{
    // Check if links are already connected
    const link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);

    if (it != _link_info_map.end()) {
        // Muxing links, add to mux ref count
        it->second.mux_ref_count++;
        return it->second.io_srv;
    }

    // Links are not muxed, create a new inline I/O service
    auto io_srv = inline_io_service::make();

    if (recv_link) {
        io_srv->attach_recv_link(recv_link);
    }
    if (send_link) {
        io_srv->attach_send_link(send_link);
    }

    _link_info_map[links] = {io_srv, 1};
    return io_srv;
}

void inline_io_service_mgr::disconnect_links(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link)
{
    const link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);
    UHD_ASSERT_THROW(it != _link_info_map.end());

    it->second.mux_ref_count--;
    if (it->second.mux_ref_count == 0) {
        if (recv_link) {
            it->second.io_srv->detach_recv_link(recv_link);
        }
        if (send_link) {
            it->second.io_srv->detach_send_link(send_link);
        }

        _link_info_map.erase(it);
    }
}

/* Blocking I/O service manager
 *
 * I/O service manager for offload I/O services configured to block. This
 * manager creates one offload I/O service for each transport adapter used by
 * a streamer. If there are multiple streamers, this manager creates a separate
 * set of I/O services for each streamer.
 */
class blocking_io_service_mgr
{
public:
    io_service::sptr connect_links(recv_link_if::sptr recv_link,
        send_link_if::sptr send_link,
        const link_type_t link_type,
        const io_service_args_t& args,
        const std::string& streamer_id);

    void disconnect_links(recv_link_if::sptr recv_link, send_link_if::sptr send_link);

private:
    struct link_info_t
    {
        std::string streamer_id;
        adapter_id_t adapter_id;
    };
    struct streamer_info_t
    {
        adapter_id_t adapter_id;
        io_service::sptr io_srv;
        size_t connection_count;
    };
    using streamer_map_key_t = std::pair<std::string, adapter_id_t>;

    io_service::sptr _create_new_io_service(const io_service_args_t& args,
        const link_type_t link_type,
        const size_t thread_index);

    // Map of links to streamer, so we can look up an I/O service from links
    using link_pair_t = std::pair<recv_link_if::sptr, send_link_if::sptr>;
    std::map<link_pair_t, link_info_t> _link_info_map;

    // Map of streamer to its I/O services
    std::map<std::string, std::vector<streamer_info_t>> _streamer_info_map;
};

io_service::sptr blocking_io_service_mgr::connect_links(recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const link_type_t link_type,
    const io_service_args_t& args,
    const std::string& streamer_id)
{
    UHD_ASSERT_THROW(
        link_type == link_type_t::RX_DATA || link_type == link_type_t::TX_DATA);

    auto adapter_id = (link_type == link_type_t::RX_DATA)
                          ? recv_link->get_recv_adapter_id()
                          : send_link->get_send_adapter_id();

    link_pair_t links = {recv_link, send_link};
    if (_link_info_map.find(links) != _link_info_map.end()) {
        throw uhd::runtime_error("Block option on offload thread is not "
                                 "supported when the transport multiplexes links.");
    }

    // If this streamer doesn't have an entry, create one
    if (_streamer_info_map.count(streamer_id) == 0) {
        _streamer_info_map[streamer_id] = {};
        _link_info_map[links]           = {streamer_id, adapter_id};
    }

    // Look for whether this streamer already has an I/O service for the same
    // adapter. If it does, then use it, otherwise create a new one.
    io_service::sptr io_srv;
    auto& info_vtr = _streamer_info_map.at(streamer_id);
    auto it        = std::find_if(
        info_vtr.begin(), info_vtr.end(), [adapter_id](const streamer_info_t& info) {
            return adapter_id == info.adapter_id;
        });

    if (it == info_vtr.end()) {
        const size_t new_thread_index = info_vtr.size();
        io_srv = _create_new_io_service(args, link_type, new_thread_index);
        info_vtr.push_back({adapter_id, io_srv, 1 /*connection_count*/});
    } else {
        it->connection_count++;
        io_srv = it->io_srv;
    }

    if (recv_link) {
        io_srv->attach_recv_link(recv_link);
    }
    if (send_link) {
        io_srv->attach_send_link(send_link);
    }

    return io_srv;
}

void blocking_io_service_mgr::disconnect_links(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link)
{
    const link_pair_t links{recv_link, send_link};
    auto link_info = _link_info_map.at(links);

    // Find the streamer_info using the streamer_id and adapter_id in link_info
    auto& info_vtr = _streamer_info_map.at(link_info.streamer_id);
    auto it        = std::find_if(info_vtr.begin(),
        info_vtr.end(),
        [adapter_id = link_info.adapter_id](
            const streamer_info_t& info) { return adapter_id == info.adapter_id; });

    UHD_ASSERT_THROW(it != info_vtr.end());

    // Detach links and decrement the connection count in streamer_info
    if (recv_link) {
        it->io_srv->detach_recv_link(recv_link);
    }
    if (send_link) {
        it->io_srv->detach_send_link(send_link);
    }

    it->connection_count--;
    if (it->connection_count == 0) {
        it->io_srv.reset();
    }

    // If all I/O services in the streamers are disconnected, clean up all its info
    bool still_in_use = false;
    for (auto info : info_vtr) {
        still_in_use |= bool(info.io_srv);
    }

    if (!still_in_use) {
        _streamer_info_map.erase(link_info.streamer_id);
    }

    // These links should no longer be connected to any I/O service
    _link_info_map.erase(links);
}

io_service::sptr blocking_io_service_mgr::_create_new_io_service(
    const io_service_args_t& args, const link_type_t link_type, const size_t thread_index)
{
    offload_io_service::params_t params;
    params.wait_mode   = offload_io_service::BLOCK;
    params.client_type = (link_type == link_type_t::RX_DATA)
                             ? offload_io_service::RECV_ONLY
                             : offload_io_service::SEND_ONLY;

    const auto& cpu_map = (link_type == link_type_t::RX_DATA)
                              ? args.recv_offload_thread_cpu
                              : args.send_offload_thread_cpu;

    std::string cpu_affinity_str;
    if (cpu_map.count(thread_index) != 0) {
        const size_t cpu         = cpu_map.at(thread_index);
        params.cpu_affinity_list = {cpu};
        cpu_affinity_str         = ", cpu affinity: " + std::to_string(cpu);
    } else {
        cpu_affinity_str = ", cpu affinity: none";
    }

    std::string link_type_str = (link_type == link_type_t::RX_DATA) ? "RX data"
                                                                    : "TX data";

    UHD_LOG_INFO(LOG_ID,
        "Creating new blocking I/O service for " << link_type_str << cpu_affinity_str);

    return offload_io_service::make(inline_io_service::make(), params);
}

/* Polling I/O service manager
 *
 * I/O service manager for offload I/O services configured to poll. Creates the
 * number of I/O services specified by the user in stream_args, and distributes
 * links among them. New connections always go to the offload thread containing
 * the fewest connections, with lowest numbered thread as a second criterion.
 */
class polling_io_service_mgr
{
public:
    io_service::sptr connect_links(recv_link_if::sptr recv_link,
        send_link_if::sptr send_link,
        const io_service_args_t& args);

    void disconnect_links(recv_link_if::sptr recv_link, send_link_if::sptr send_link);

private:
    struct link_info_t
    {
        io_service::sptr io_srv;
        size_t mux_ref_count;
    };
    struct io_srv_info_t
    {
        size_t connection_count;
    };

    io_service::sptr _create_new_io_service(
        const io_service_args_t& args, const size_t thread_index);

    // Map of links to I/O service
    using link_pair_t = std::pair<recv_link_if::sptr, send_link_if::sptr>;
    std::map<link_pair_t, link_info_t> _link_info_map;

    // For each I/O service, keep track of the number of connections
    std::map<io_service::sptr, io_srv_info_t> _io_srv_info_map;
};

io_service::sptr polling_io_service_mgr::connect_links(recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const io_service_args_t& args)
{
    // Check if links are already connected
    const link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);
    if (it != _link_info_map.end()) {
        // Muxing links, add to mux ref count and connection count
        it->second.mux_ref_count++;
        _io_srv_info_map[it->second.io_srv].connection_count++;
        return it->second.io_srv;
    }

    // Links are not muxed. If there are fewer offload threads than requested in
    // the args, create a new service and add the links to it. Otherwise, add it
    // to the service that has the fewest connections.
    io_service::sptr io_srv;
    if (_io_srv_info_map.size() < args.num_poll_offload_threads) {
        const size_t thread_index = _io_srv_info_map.size();
        io_srv                    = _create_new_io_service(args, thread_index);
        _link_info_map[links]     = {io_srv, 1 /*mux_ref_count*/};
        _io_srv_info_map[io_srv]  = {1 /*connection_count*/};
    } else {
        using map_pair_t = std::pair<io_service::sptr, io_srv_info_t>;
        auto cmp         = [](const map_pair_t& left, const map_pair_t& right) {
            return left.second.connection_count < right.second.connection_count;
        };

        auto it = std::min_element(_io_srv_info_map.begin(), _io_srv_info_map.end(), cmp);
        UHD_ASSERT_THROW(it != _io_srv_info_map.end());
        io_srv = it->first;
        _io_srv_info_map[io_srv].connection_count++;
    }

    if (recv_link) {
        io_srv->attach_recv_link(recv_link);
    }
    if (send_link) {
        io_srv->attach_send_link(send_link);
    }
    return io_srv;
}

void polling_io_service_mgr::disconnect_links(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link)
{
    const link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);
    UHD_ASSERT_THROW(it != _link_info_map.end());

    auto io_srv = it->second.io_srv;
    it->second.mux_ref_count--;

    if (it->second.mux_ref_count == 0) {
        if (recv_link) {
            io_srv->detach_recv_link(recv_link);
        }
        if (send_link) {
            io_srv->detach_send_link(send_link);
        }

        _link_info_map.erase(it);
        _io_srv_info_map.erase(io_srv);
    }
}

io_service::sptr polling_io_service_mgr::_create_new_io_service(
    const io_service_args_t& args, const size_t thread_index)
{
    offload_io_service::params_t params;
    params.client_type = offload_io_service::BOTH_SEND_AND_RECV;
    params.wait_mode   = offload_io_service::POLL;

    const auto& cpu_map = args.poll_offload_thread_cpu;

    std::string cpu_affinity_str;
    if (cpu_map.count(thread_index) != 0) {
        const size_t cpu         = cpu_map.at(thread_index);
        params.cpu_affinity_list = {cpu};
        cpu_affinity_str         = ", cpu affinity: " + std::to_string(cpu);
    } else {
        cpu_affinity_str = ", cpu affinity: none";
    }

    UHD_LOG_INFO(LOG_ID, "Creating new polling I/O service" << cpu_affinity_str);

    return offload_io_service::make(inline_io_service::make(), params);
}

/* Main I/O service manager implementation class
 *
 * Composite I/O service manager that dispatches requests to other managers,
 * based on transport args and link type.
 */
class io_service_mgr_impl : public io_service_mgr
{
public:
    io_service_mgr_impl(const uhd::device_addr_t& args) : _args(args) {}

    io_service::sptr connect_links(recv_link_if::sptr recv_link,
        send_link_if::sptr send_link,
        const link_type_t link_type,
        const io_service_args_t& default_args,
        const uhd::device_addr_t& stream_args,
        const std::string& streamer_id) override;

    void disconnect_links(
        recv_link_if::sptr recv_link, send_link_if::sptr send_link) override;

private:
    enum io_service_type_t { INLINE_IO_SRV, BLOCKING_IO_SRV, POLLING_IO_SRV };
    struct xport_args_t
    {
        bool offload                              = false;
        offload_io_service::wait_mode_t wait_mode = offload_io_service::BLOCK;
    };
    struct link_info_t
    {
        io_service::sptr io_srv;
        io_service_type_t io_srv_type;
    };
    using link_pair_t = std::pair<recv_link_if::sptr, send_link_if::sptr>;

    bool _out_of_order_supported(
        recv_link_if::sptr recv_link, send_link_if::sptr send_link) const;

    const uhd::device_addr_t _args;

    inline_io_service_mgr _inline_io_srv_mgr;
    blocking_io_service_mgr _blocking_io_srv_mgr;
    polling_io_service_mgr _polling_io_srv_mgr;

    // Map of links to I/O service
    std::map<link_pair_t, link_info_t> _link_info_map;
};

io_service_mgr::sptr io_service_mgr::make(const uhd::device_addr_t& args)
{
    constrained_device_args_t::bool_arg use_dpdk("use_dpdk", false);
    if (args.has_key(use_dpdk.key())) {
        use_dpdk.parse(args[use_dpdk.key()]);
    }

    if (use_dpdk.get()) {
#ifdef HAVE_DPDK
        return std::make_shared<dpdk_io_service_mgr_impl>();
#else
        UHD_LOG_WARNING(LOG_ID,
            "Cannot instantiate DPDK I/O service. Proceeding with regular I/O service.");
#endif
    }
    return std::make_shared<io_service_mgr_impl>(args);
}

io_service::sptr io_service_mgr_impl::connect_links(recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    const link_type_t link_type,
    const io_service_args_t& default_args_,
    const uhd::device_addr_t& stream_args,
    const std::string& streamer_id)
{
    UHD_ASSERT_THROW(link_type != link_type_t::ASYNC_MSG);

    io_service_args_t default_args = default_args_;

    if (!_out_of_order_supported(recv_link, send_link)) {
        default_args.recv_offload = false;
        default_args.send_offload = false;
    }

    const io_service_args_t args =
        read_io_service_args(merge_io_service_dev_args(_args, stream_args), default_args);

    // Check if the links are already attached to an I/O service. If they are,
    // then use the same manager to connect, since links can only be connected
    // to one I/O service at any given a time.
    link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);

    io_service::sptr io_srv;
    io_service_type_t io_srv_type;

    if (it != _link_info_map.end()) {
        io_srv      = it->second.io_srv;
        io_srv_type = it->second.io_srv_type;
    } else {
        // Links not already attached, pick an io_service_mgr to connect based
        // on user parameters and connect them.
        if (link_type == link_type_t::CTRL) {
            io_srv_type = INLINE_IO_SRV;
        } else {
            bool offload   = (link_type == link_type_t::RX_DATA) ? args.recv_offload
                                                                 : args.send_offload;
            auto wait_mode = (link_type == link_type_t::RX_DATA)
                                 ? args.recv_offload_wait_mode
                                 : args.send_offload_wait_mode;

            if (offload) {
                if (wait_mode == io_service_args_t::POLL) {
                    io_srv_type = POLLING_IO_SRV;
                } else {
                    io_srv_type = BLOCKING_IO_SRV;
                }
            } else {
                io_srv_type = INLINE_IO_SRV;
            }
        }
    }

    // If the link doesn't support buffers out of order, then we can only use
    // the inline I/O service. Warn if a different one was requested.
    if (!_out_of_order_supported(recv_link, send_link)) {
        if (io_srv_type != INLINE_IO_SRV) {
            UHD_LOG_WARNING(
                LOG_ID, "Link type does not support send/recv offload, ignoring");
        }
        io_srv_type = INLINE_IO_SRV;
    }

    switch (io_srv_type) {
        case INLINE_IO_SRV:
            io_srv = _inline_io_srv_mgr.connect_links(recv_link, send_link);
            break;
        case BLOCKING_IO_SRV:
            io_srv = _blocking_io_srv_mgr.connect_links(
                recv_link, send_link, link_type, args, streamer_id);
            break;
        case POLLING_IO_SRV:
            io_srv = _polling_io_srv_mgr.connect_links(recv_link, send_link, args);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    _link_info_map[links] = {io_srv, io_srv_type};
    return io_srv;
}

void io_service_mgr_impl::disconnect_links(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link)
{
    link_pair_t links{recv_link, send_link};
    auto it = _link_info_map.find(links);

    UHD_ASSERT_THROW(it != _link_info_map.end());
    switch (it->second.io_srv_type) {
        case INLINE_IO_SRV:
            _inline_io_srv_mgr.disconnect_links(recv_link, send_link);
            break;
        case BLOCKING_IO_SRV:
            _blocking_io_srv_mgr.disconnect_links(recv_link, send_link);
            break;
        case POLLING_IO_SRV:
            _polling_io_srv_mgr.disconnect_links(recv_link, send_link);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    _link_info_map.erase(it);
}

bool io_service_mgr_impl::_out_of_order_supported(
    recv_link_if::sptr recv_link, send_link_if::sptr send_link) const
{
    bool supported = true;
    if (recv_link) {
        supported = recv_link->supports_recv_buff_out_of_order();
    }
    if (send_link) {
        supported = supported && send_link->supports_send_buff_out_of_order();
    }
    return supported;
}

}} // namespace uhd::usrp
