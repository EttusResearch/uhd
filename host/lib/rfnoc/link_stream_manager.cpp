//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <uhdlib/rfnoc/link_stream_manager.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <boost/format.hpp>
#include <map>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::transport;
using namespace uhd::rfnoc::chdr;
using namespace uhd::rfnoc::mgmt;
using namespace uhd::rfnoc::detail;

namespace {

constexpr sep_inst_t SEP_INST_MGMT_CTRL = 0;
constexpr sep_inst_t SEP_INST_DATA_BASE = 1;

constexpr double STREAM_SETUP_TIMEOUT = 0.2;

constexpr char LOG_ID[] = "RFNOC::LSM";

}

link_stream_manager::~link_stream_manager() = default;

class link_stream_manager_impl : public link_stream_manager
{
public:
    link_stream_manager_impl(const chdr::chdr_packet_factory& pkt_factory,
        mb_iface& mb_if,
        const epid_allocator::sptr& epid_alloc,
        device_id_t device_id,
        topo_graph_t::sptr topo_graph)
        : _pkt_factory(pkt_factory)
        , _my_device_id(device_id)
        , _mb_iface(mb_if)
        , _epid_alloc(epid_alloc)
        , _data_ep_inst(0)
        , _tgraph(std::move(topo_graph))
    {
        // Sanity check if we can access our device ID from this motherboard
        const auto& mb_devs = _mb_iface.get_local_device_ids();
        if (std::find(mb_devs.begin(), mb_devs.end(), _my_device_id) == mb_devs.end()) {
            throw uhd::rfnoc_error("The device bound to this link manager cannot be "
                                   "accessed from this motherboard");
        }

        // Sanity check the protocol version and CHDR width
        if ((_pkt_factory.get_protover() & 0xFF00)
            != (_mb_iface.get_proto_ver() & 0xFF00)) {
            throw uhd::rfnoc_error("RFNoC protocol mismatch between SW and HW");
        }
        if (_pkt_factory.get_chdr_w() != _mb_iface.get_chdr_w()) {
            throw uhd::rfnoc_error("RFNoC CHDR width mismatch between SW and HW");
        }

        // Create a transport and EPID for management and control traffic
        _my_mgmt_ctrl_epid =
            epid_alloc->allocate_epid(sep_addr_t(_my_device_id, SEP_INST_MGMT_CTRL));
        _allocated_epids.insert(_my_mgmt_ctrl_epid);

        // Create a muxed transport to share between the mgmt_portal and
        // chdr_ctrl_endpoint. We have to use the same base transport here to ensure that
        // the route setup logic in the FPGA transport works correctly.
        _ctrl_xport = _mb_iface.make_ctrl_transport(_my_device_id, _my_mgmt_ctrl_epid);

        _my_adapter_id = _mb_iface.get_adapter_id(_my_device_id);

        // Create management portal using one of the child transports. This also
        // runs the topology discovery.
        _mgmt_portal = mgmt_portal::make(*_ctrl_xport,
            _pkt_factory,
            sep_addr_t(_my_device_id, SEP_INST_MGMT_CTRL),
            _tgraph);
    }

    void add_unreachable_transport_adapters() override
    {
        // Topology discovery can't detect transport adapters that are not also
        // connected to UHD, but we might need those for remote streams. We
        // therefore query a list of available transport adapters we need to
        // know about and slot them into the graph using some knowledge we have
        // of RFNoC internals. This is not an ideal and super-robust solution,
        // but it is backward-compatible. Future updates to RFNoC might remove
        // the need to do things this way.
        const device_id_t mb_dev_id = _mb_iface.get_remote_device_id();
        for (auto& ta : _mb_iface.get_chdr_xport_adapters()) {
            const sep_inst_t ta_inst = ta.second.cast<sep_inst_t>("ta_inst", -1);
            detail::topo_node_t ta_node(
                mb_dev_id, detail::topo_node_t::node_type::XPORT, ta_inst, 0);
            if (_tgraph->add_node(ta_node)) {
                UHD_LOG_DEBUG(LOG_ID,
                    "Adding node " << ta_node.to_string()
                                   << " to topology graph outside of discovery.");
                const auto xbar_nodes = _tgraph->get_nodes([&](const topo_node_t& node) {
                    return node.type == detail::topo_node_t::node_type::XBAR
                           && node.device_id == mb_dev_id;
                });
                UHD_ASSERT_THROW(xbar_nodes.size() < 2);
                if (xbar_nodes.empty()) {
                    UHD_LOG_DEBUG("RFNOC",
                        "Cannot link transport adapter node " << ta_node.to_string()
                                                              << " to topology.");
                    continue;
                }
                detail::topo_edge_t edge;
                edge.type = detail::topo_edge_t::edge_type::ON_CHIP;
                // This equality is just something we happen to do in the HDL,
                // but it's the only way to slot in these transport adapters
                // without a better topology discovery.
                edge.src_port = ta_node.inst;
                UHD_LOG_DEBUG(
                    LOG_ID, "Adding transport adapter on xbar port " << edge.src_port);
                _tgraph->add_biedge(xbar_nodes.front(), ta_node, edge);
            }
        }
    }

    ~link_stream_manager_impl() override
    {
        for (const auto& epid : _allocated_epids) {
            _epid_alloc->deallocate_epid(epid);
        }
    }

    device_id_t get_self_device_id() const override
    {
        return _my_device_id;
    }

    uhd::transport::adapter_id_t get_adapter_id() const override
    {
        return _my_adapter_id;
    }

    std::set<sep_addr_t> get_reachable_endpoints() const override
    {
        // FIXME this becomes a graph lookup
        return _mgmt_portal->get_reachable_endpoints();
    }

    bool can_connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) const override
    {
        return _mgmt_portal->can_remote_route(dst_addr, src_addr);
    }

    sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr) override
    {
        _ensure_ep_is_reachable(dst_addr);

        // Allocate EPIDs
        sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);

        // Make sure that the software side of the endpoint is initialized and reachable
        if (_ctrl_ep == nullptr) {
            // Create a control endpoint with that xport
            _ctrl_ep =
                chdr_ctrl_endpoint::make(_ctrl_xport, _pkt_factory, _my_mgmt_ctrl_epid);
        }

        // Setup a route to the EPID
        _mgmt_portal->setup_local_route(*_ctrl_xport, dst_epid);
        if (!_mgmt_portal->get_endpoint_info(dst_epid).has_ctrl) {
            throw uhd::rfnoc_error(
                "Downstream endpoint does not support control traffic");
        }

        // Create a client zero instance
        if (_client_zero_map.count(dst_epid) == 0) {
            _client_zero_map.insert(
                std::make_pair(dst_epid, client_zero::make(*_ctrl_ep, dst_epid)));
        }
        return sep_id_pair_t(_my_mgmt_ctrl_epid, dst_epid);
    }

    sep_id_pair_t connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) override
    {
        _ensure_ep_is_reachable(dst_addr);
        _ensure_ep_is_reachable(src_addr);

        // Allocate EPIDs and initialize endpoints
        sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);
        sep_id_t src_epid =
            _epid_alloc->allocate_epid(src_addr, *_mgmt_portal, *_ctrl_xport);

        // Set up routes
        _mgmt_portal->setup_remote_route(*_ctrl_xport, dst_epid, src_epid);

        return sep_id_pair_t(src_epid, dst_epid);
    }

    ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk) override
    {
        // Ensure that the endpoint is initialized for control at the specified EPID
        if (_ctrl_ep == nullptr) {
            throw uhd::runtime_error("Software endpoint not initialized for control");
        }
        if (_client_zero_map.count(dst_epid) == 0) {
            throw uhd::runtime_error(
                "Control for the specified EPID was not initialized");
        }
        const client_zero::sptr& c0_ctrl = _client_zero_map.at(dst_epid);
        const uint16_t block_slot = 1 + c0_ctrl->get_num_stream_endpoints() + block_index;
        if (block_index >= c0_ctrl->get_num_blocks()) {
            throw uhd::value_error("Requested block index out of range");
        }

        // Create control endpoint
        return _ctrl_ep->get_ctrlport_ep(dst_epid,
            c0_ctrl->get_ctrl_xbar_port(block_index),
            (size_t(1) << c0_ctrl->get_block_info(block_slot).ctrl_fifo_size),
            c0_ctrl->get_block_info(block_slot).ctrl_max_async_msgs,
            client_clk,
            timebase_clk);
    }

    client_zero::sptr get_client_zero(sep_id_t dst_epid) const override
    {
        if (_client_zero_map.count(dst_epid) == 0) {
            throw uhd::runtime_error(
                "Control for the specified EPID was not initialized");
        }
        return _client_zero_map.at(dst_epid);
    }

    stream_buff_params_t create_device_to_device_data_stream(const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const bool reset = false) override
    {
        // We assume that the devices are already connected (because this API requires
        // EPIDs)

        // Setup a stream
        stream_buff_params_t buff_params =
            _mgmt_portal->config_remote_stream(*_ctrl_xport,
                dst_epid,
                src_epid,
                lossy_xport,
                stream_buff_params_t{1, 1}, // Dummy frequency
                stream_buff_params_t{0, 0}, // Dummy headroom
                false,
                STREAM_SETUP_TIMEOUT);

        // Reconfigure flow control using the new frequency and headroom
        return _mgmt_portal->config_remote_stream(*_ctrl_xport,
            dst_epid,
            src_epid,
            lossy_xport,
            _get_buff_params_ratio(buff_params, fc_freq_ratio),
            _get_buff_params_ratio(buff_params, fc_headroom_ratio),
            reset,
            STREAM_SETUP_TIMEOUT);
    }

    chdr_tx_data_xport::uptr create_host_to_device_data_stream(const sep_addr_t dst_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        _ensure_ep_is_reachable(dst_addr);

        // Generate a new destination (device) EPID instance
        const sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);

        if (!_mgmt_portal->get_endpoint_info(dst_epid).has_data) {
            throw uhd::rfnoc_error("Downstream endpoint does not support data traffic");
        }

        // Create a new source (host) endpoint and EPID
        {
            std::lock_guard<std::mutex> ep_lock(_data_ep_lock);
            if (_data_src_ep_map.count(dst_addr) == 0) {
                _data_src_ep_map[dst_addr] = SEP_INST_DATA_BASE + _data_ep_inst++;
            }
        }
        const sep_addr_t sw_epid_addr(_my_device_id, _data_src_ep_map[dst_addr]);
        const sep_id_t src_epid = _epid_alloc->allocate_epid(sw_epid_addr);
        _allocated_epids.insert(src_epid);

        return _mb_iface.make_tx_data_transport(*_mgmt_portal,
            {sw_epid_addr, dst_addr},
            {src_epid, dst_epid},
            pyld_buff_fmt,
            mdata_buff_fmt,
            xport_args,
            streamer_id);
    }

    chdr_rx_data_xport::uptr create_device_to_host_data_stream(sep_addr_t src_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        _ensure_ep_is_reachable(src_addr);

        // Generate a new source (device) EPID instance
        const sep_id_t src_epid =
            _epid_alloc->allocate_epid(src_addr, *_mgmt_portal, *_ctrl_xport);

        if (!_mgmt_portal->get_endpoint_info(src_epid).has_data) {
            throw uhd::rfnoc_error("Downstream endpoint does not support data traffic");
        }

        // Create a new destination (host) endpoint and EPID
        {
            std::lock_guard<std::mutex> ep_lock(_data_ep_lock);
            if (_data_dst_ep_map.count(src_addr) == 0) {
                _data_dst_ep_map[src_addr] = SEP_INST_DATA_BASE + _data_ep_inst++;
            }
        }
        const sep_addr_t sw_epid_addr(_my_device_id, _data_dst_ep_map[src_addr]);
        const sep_id_t dst_epid = _epid_alloc->allocate_epid(sw_epid_addr);
        _allocated_epids.insert(dst_epid);

        // Sanitize xport args
        const device_addr_t sanitized_xport_args = _sanitize_xport_args(xport_args);
        // Create the data xport. Note that his also sets up the remote SEP and
        // configures the route between the remote and local SEP.
        auto data_xport = _mb_iface.make_rx_data_transport(*_mgmt_portal,
            {src_addr, sw_epid_addr},
            {src_epid, dst_epid},
            pyld_buff_fmt,
            mdata_buff_fmt,
            sanitized_xport_args,
            streamer_id);

        // At this point, the remote SEP is configured, knows its own EPID, and
        // we have programmed a route from data_xport to the SEP and (more
        // importantly) from the remote SEP to data_xport.
        // If we want to divert the stream to a remote destination, then we need
        // to do some additional configuration.
        if (sanitized_xport_args.get("enable_remote_stream", "0") == "1") {
            UHD_LOG_TRACE(LOG_ID, "Creating remote RX stream...");
            _enable_remote_rx_stream(sanitized_xport_args, src_addr, dst_epid);
        }

        return data_xport;
    }

private:
    void _ensure_ep_is_reachable(const sep_addr_t& ep_addr_)
    {
        for (const auto& ep_addr : _mgmt_portal->get_reachable_endpoints()) {
            if (ep_addr == ep_addr_)
                return;
        }
        throw uhd::routing_error("Specified endpoint is not reachable");
    }

    stream_buff_params_t _get_buff_params_ratio(
        const stream_buff_params_t& buff_params, const double ratio)
    {
        return {static_cast<uint64_t>(std::ceil(double(buff_params.bytes) * ratio)),
            static_cast<uint32_t>(std::ceil(double(buff_params.packets) * ratio))};
    }

    /*! Sanitize xport args for RX stream generation
     *
     * This is called by create_device_to_host_data_stream().
     *
     * Notable rules:
     * - After this call, 'enable_remote_stream' is either '0' or '1'.
     * - If 'adapter' is provided, it is checked for validity.
     * - If 'dest_addr' is provided, we check that 'dest_port' is also provided and vice
     *   versa.
     * - 'stream_mode' and 'enable_fc' will be set to sensible defaults if not
     *   provided.
     */
    device_addr_t _sanitize_xport_args(const device_addr_t& xport_args)
    {
        auto args = xport_args;

        // Check for UDP-based stream diversion
        if (args.has_key("dest_addr")) {
            auto available_adapters = _mb_iface.get_chdr_xport_adapters();
            if (available_adapters.empty()) {
                throw uhd::value_error(
                    "Device has no advanced transport adapters for remote streaming!");
            }
            args["enable_remote_stream"] = "1";
            if (!args.has_key("dest_port")) {
                throw uhd::value_error(
                    "Missing `dest_port' argument for remote streaming destination!");
            }
            // We default flow control for remote links to 'off', because the
            // most common use case is the 'fire and forget' one.
            if (!args.has_key("enable_fc")) {
                args["enable_fc"] = "0";
            }
            if (!args.has_key("stream_mode")) {
                args["stream_mode"] = "raw_payload";
            } else {
                if (args["stream_mode"] != "raw_payload" && args["stream_mode"] != "full_packet") {
                    UHD_LOG_THROW(uhd::runtime_error,
                        LOG_ID,
                        "Invalid stream mode: " << args["stream_mode"]);
                }
            }
            if (args.has_key("adapter")) {
                // check this is a valid adapter
                if (!available_adapters.count(args["adapter"])) {
                    throw uhd::value_error(
                        "Unknown adapter identifier: " + args["adapter"]);
                }
            }
        } else if (args.has_key("dest_port")) {
            throw uhd::value_error(
                "Missing `dest_addr' argument for remote streaming destination!");
        } else {
            args["enable_remote_stream"] = "0";
        }
        return args;
    }

    /*! Configures an RX stream (from device_to_host) to be diverted to a
     * different target.
     *
     * This makes sure that the routing tables in the device are set to route
     * the data packets to the remote location, and the transport adapter knows
     * where to send the outgoing packets to.
     */
    void _enable_remote_rx_stream(
            const device_addr_t& xport_args,
            const sep_addr_t src_addr,
            const sep_id_t dst_epid)
    {
        // Get the route from the regular streamer to the SEP and see which
        // transport adapter we're using
        const auto route_to_sep = _mgmt_portal->get_route(src_addr);
        auto route_it = route_to_sep.begin();
        UHD_ASSERT_THROW(!!route_it->node.epid);
        const sep_id_t src_epid = route_it->node.epid.get();
        route_it++;
        UHD_ASSERT_THROW(route_it->node.type == topo_node_t::node_type::XPORT);
        auto ta_node = route_it->node;
        const auto default_ta_inst = ta_node.inst;
        UHD_LOG_TRACE(LOG_ID,
            "Configuring remote stream via transport adapter: " << ta_node.to_string());

        const auto available_transport_adapters = _mb_iface.get_chdr_xport_adapters();
        // If the user specified an adapter, check if we're on that route, or if
        // we need to program a separate one
        std::string remote_adapter_id = xport_args.get("adapter", "");
        if (xport_args.has_key("adapter")) {
            // We assume xport_args are sanitized, so we know this lookup should not
            // throw an exception.
            const auto ta_info = available_transport_adapters.at(remote_adapter_id);
            ta_node.inst  = ta_info.cast<sep_inst_t>("ta_inst", -1);
            if (!ta_info.has_key("rx_routing")) {
                UHD_LOG_THROW(uhd::runtime_error,
                    LOG_ID,
                    "Requested remote UDP streaming, but transport adapter "
                        << remote_adapter_id << " does not support it!");
            }
        } else {
            for (auto& ta : available_transport_adapters) {
                if (ta.second.cast<sep_inst_t>("ta_inst", -1) == default_ta_inst) {
                    remote_adapter_id = ta.first;
                    ta_node.inst = ta.second.cast<sep_inst_t>("ta_inst", -1);
                }
            }
            if (remote_adapter_id.empty()) {
                UHD_LOG_THROW(uhd::runtime_error,
                    LOG_ID,
                    "Cannot identify transport adapter "
                        << default_ta_inst << " on route to EPID " << dst_epid);
            }
        }
        UHD_ASSERT_THROW(ta_node.inst != sep_inst_t(-1));
        if (xport_args.get("stream_mode") == "raw_payload"
            && !available_transport_adapters.at(remote_adapter_id)
                    .has_key("rx_hdr_removal")) {
            UHD_LOG_THROW(uhd::runtime_error,
                LOG_ID,
                "Requested to remove headers on transport adapter "
                    << remote_adapter_id << ", but adapter does not support it!");
        }
        UHD_LOG_TRACE(LOG_ID,
            "Using transport adapter " << remote_adapter_id << " (node instance: "
                                       << ta_node.inst << ") for remote RX stream.");

        if (ta_node.inst != default_ta_inst) {
            UHD_LOG_DEBUG(LOG_ID,
                "Remote stream is using different transport adapter ("
                    << ta_node.to_string()
                    << ") than local streamer object. Setting up remote route...");
            // Add a virtual EP after this TA. This is so the topology graph
            // has a notion of the remote data receiver, even if that's not part
            // of UHD.
            const detail::topo_node_t virtual_ep(_my_device_id,
                detail::topo_node_t::node_type::VIRTUAL,
                dst_epid,
                0,
                dst_epid);
            UHD_LOG_DEBUG(LOG_ID,
                "Adding virtual endpoint: " << virtual_ep.to_string() << " (EPID: "
                                            << virtual_ep.epid.get() << ")");
            detail::topo_edge_t virtual_edge;
            virtual_edge.type = detail::topo_edge_t::edge_type::ETHERNET;
            _tgraph->add_edge(ta_node, virtual_ep, virtual_edge);
            _mgmt_portal->setup_remote_route(*_ctrl_xport, dst_epid, src_epid);
        }
        // There is no 'else' clause. If the TA was en route to dst_epid, then
        // we've already programmed all the routes between the SEP and the TA.
        // The only thing left to do is to program the destination IP/port into
        // the TA.
        UHD_LOG_DEBUG("RFNOC",
            "Creating diverted RX stream with arguments: " << xport_args.to_string());
        _mb_iface.add_remote_chdr_route(remote_adapter_id, dst_epid, xport_args);
    }

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    // A reference to the packet factory
    const chdr::chdr_packet_factory& _pkt_factory;
    // The device address of this software endpoint
    const device_id_t _my_device_id;
    // The host adapter ID associated with this software endpoint
    adapter_id_t _my_adapter_id;

    // Motherboard interface
    mb_iface& _mb_iface;
    // A pointer to the EPID allocator
    epid_allocator::sptr _epid_alloc;
    // A set of all allocated EPIDs
    std::set<sep_id_t> _allocated_epids;
    // The software EPID for all management and control traffic
    sep_id_t _my_mgmt_ctrl_epid;
    // Transports
    chdr_ctrl_xport::sptr _ctrl_xport;
    // Management portal for control endpoints
    mgmt_portal::uptr _mgmt_portal;
    // The CHDR control endpoint
    chdr_ctrl_endpoint::uptr _ctrl_ep;
    // A map of all client zero instances indexed by the destination
    std::map<sep_id_t, client_zero::sptr> _client_zero_map;

    // Data endpoint mutex
    std::mutex _data_ep_lock;
    // Data endpoint instance
    sep_inst_t _data_ep_inst;

    // Maps to cache local data endpoints for re-use.  Assumes each connection
    // between the host and a stream endpoint on a given device is unique for
    // each direction.  Re-using enpdoints is needed because the routing table
    // in the FPGA is limited in how many entries can be made.
    std::map<sep_addr_t, sep_inst_t> _data_src_ep_map;
    std::map<sep_addr_t, sep_inst_t> _data_dst_ep_map;

    // Reference to the topology graph. Note that this is owned by
    // graph_stream_manager_impl, but we get a reference for convenience sake.
    topo_graph_t::sptr _tgraph;
};

link_stream_manager::uptr link_stream_manager::make(
    const chdr::chdr_packet_factory& pkt_factory,
    mb_iface& mb_if,
    const epid_allocator::sptr& epid_alloc,
    device_id_t device_id,
    topo_graph_t::sptr topo_graph)
{
    return std::make_unique<link_stream_manager_impl>(
        pkt_factory, mb_if, epid_alloc, device_id, std::move(topo_graph));
}
