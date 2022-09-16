//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <cmath>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace uhd { namespace rfnoc { namespace mgmt {

using namespace chdr;
using namespace transport;
using namespace uhd::rfnoc::detail;


namespace {

constexpr char LOG_ID[] = "RFNOC::MGMT";

constexpr bool ALLOW_DAISY_CHAINING = true;

// Unused values are left in as comments for reference.
constexpr uint16_t REG_EPID_SELF               = 0x00; // RW
constexpr uint16_t REG_RESET_AND_FLUSH         = 0x04; // W
constexpr uint16_t REG_OSTRM_CTRL_STATUS       = 0x08; // RW
constexpr uint16_t REG_OSTRM_DST_EPID          = 0x0C; // W
constexpr uint16_t REG_OSTRM_FC_FREQ_BYTES_LO  = 0x10; // W
constexpr uint16_t REG_OSTRM_FC_FREQ_BYTES_HI  = 0x14; // W
constexpr uint16_t REG_OSTRM_FC_FREQ_PKTS      = 0x18; // W
constexpr uint16_t REG_OSTRM_FC_HEADROOM       = 0x1C; // W
constexpr uint16_t REG_OSTRM_BUFF_CAP_BYTES_LO = 0x20; // R
constexpr uint16_t REG_OSTRM_BUFF_CAP_BYTES_HI = 0x24; // R
constexpr uint16_t REG_OSTRM_BUFF_CAP_PKTS     = 0x28; // R
// constexpr uint16_t REG_OSTRM_SEQ_ERR_CNT       = 0x2C; // R
// constexpr uint16_t REG_OSTRM_DATA_ERR_CNT      = 0x30; // R
// constexpr uint16_t REG_OSTRM_ROUTE_ERR_CNT     = 0x34; // R
constexpr uint16_t REG_ISTRM_CTRL_STATUS = 0x38; // RW

constexpr uint32_t RESET_AND_FLUSH_OSTRM = (1 << 0);
constexpr uint32_t RESET_AND_FLUSH_ISTRM = (1 << 1);
// constexpr uint32_t RESET_AND_FLUSH_CTRL  = (1 << 2);
constexpr uint32_t RESET_AND_FLUSH_ALL = 0x7;

#ifdef UHD_BIG_ENDIAN
constexpr endianness_t HOST_ENDIANNESS = ENDIANNESS_BIG;
#else
constexpr endianness_t HOST_ENDIANNESS = ENDIANNESS_LITTLE;
#endif

constexpr uint32_t BUILD_CTRL_STATUS_WORD(bool cfg_start,
    bool xport_lossy,
    sw_buff_t pyld_buff_fmt,
    sw_buff_t mdata_buff_fmt,
    bool byte_swap)
{
    return (cfg_start ? 1 : 0) | (xport_lossy ? 2 : 0)
           | (static_cast<uint32_t>(pyld_buff_fmt) << 2)
           | (static_cast<uint32_t>(mdata_buff_fmt) << 4) | (byte_swap ? (1 << 6) : 0);
}

constexpr uint32_t STRM_STATUS_FC_ENABLED    = 0x80000000;
constexpr uint32_t STRM_STATUS_SETUP_ERR     = 0x40000000;
constexpr uint32_t STRM_STATUS_SETUP_PENDING = 0x20000000;

} // namespace


// Empty dtor for stream_manager
mgmt_portal::~mgmt_portal() {}

//---------------------------------------------------------------
// Management Portal Implementation
//---------------------------------------------------------------
class mgmt_portal_impl : public mgmt_portal
{
public:
    mgmt_portal_impl(chdr_ctrl_xport& xport,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_addr_t my_sep_addr,
        topo_graph_t::sptr topo_graph)
        : _protover(pkt_factory.get_protover())
        , _chdr_w(pkt_factory.get_chdr_w())
        , _endianness(pkt_factory.get_endianness())
        // We use the EPID of the xport as the local instance just for convenience.
        // For the local endpoints, the 'instance' doesn't really matter because
        // we only have one endpoint per local device.
        , _my_node_id({my_sep_addr.first, xport.get_epid()}, true, xport.get_epid())
        , _send_pkt(pkt_factory.make_mgmt())
        , _recv_pkt(pkt_factory.make_mgmt())
        , _tgraph(topo_graph)
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_tgraph->add_node(_my_node_id)) {
            const std::string err_msg = "Failed to add node " + _my_node_id.to_string()
                                        + " to the topology graph! Node already exists.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::runtime_error(err_msg);
        }
        _discover_topology(xport);
        UHD_LOG_DEBUG(LOG_ID,
            "The following endpoints are reachable from " << _my_node_id.to_string());
        for (const auto& ep : get_reachable_endpoints()) {
            UHD_LOG_DEBUG(LOG_ID, "* " << ep.first << ":" << ep.second);
        }
    }

    ~mgmt_portal_impl() override {}

    std::set<sep_addr_t> get_reachable_endpoints() const override
    {
        auto reachable_ep_nodes =
            _tgraph->get_connected_nodes(_my_node_id, [](const topo_node_t& node_id) {
                return node_id.is_local_sep == false
                       && node_id.type == topo_node_t::node_type::STRM_EP;
            });
        std::set<sep_addr_t> result;
        for (auto& node : reachable_ep_nodes) {
            result.insert({node.device_id, node.inst});
        }
        return result;
    }

    void initialize_endpoint(
        chdr_ctrl_xport& xport, const sep_addr_t& addr, const sep_id_t& epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        auto my_epid = xport.get_epid();

        // Create a node ID from lookup info
        topo_node_t lookup_node(addr);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, lookup_node, my_epid);

        mgmt_hop_t cfg_hop;
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_RESET_AND_FLUSH, RESET_AND_FLUSH_ALL)));
        cfg_hop.add_op(mgmt_op_t(
            mgmt_op_t::MGMT_OP_CFG_WR_REQ, mgmt_op_t::cfg_payload(REG_EPID_SELF, epid)));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
        cfg_xact.add_hop(cfg_hop);

        // Send the transaction and receive a response.
        // We don't care about the contents of the response.
        _send_recv_mgmt_transaction(xport, cfg_xact);
        register_endpoint(addr, epid);
    }

    void register_endpoint(const sep_addr_t& addr, const sep_id_t& epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (is_endpoint_registered(epid)) {
            return;
        }
        // Create a node ID from lookup info
        topo_node_t lookup_node(addr);
        if (!_tgraph->has_route(_my_node_id, lookup_node)) {
            throw uhd::lookup_error(
                "register_endpoint(): Cannot reach node with specified address: "
                + std::to_string(addr.first) + ":" + std::to_string(addr.second));
        }
        // Add/update the EPID entry in the topo graph
        _tgraph->access_node(topo_node_t(addr)).epid = epid;
        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Bound stream endpoint with Addr=(%d,%d) to EPID=%d")
                % addr.first % addr.second % epid));
        UHD_LOG_TRACE(LOG_ID,
            (boost::format(
                 "Stream endpoint with EPID=%d can be reached by taking the path: %s")
                % epid % to_string(_tgraph->get_route(_my_node_id, lookup_node))));
    }

    bool is_endpoint_registered(const sep_id_t& epid) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        const auto nodes = _tgraph->get_nodes([epid](const topo_node_t& node) {
            return (node.type == topo_node_t::node_type::STRM_EP
                       || node.type == topo_node_t::node_type::VIRTUAL)
                   && node.epid == epid;
        });

        if (nodes.size() > 1) {
            UHD_LOG_WARNING(LOG_ID,
                "More than one endpoint with EPID " << epid << " was registered!");
        }

        return !nodes.empty();
    }

    sep_info_t get_endpoint_info(const sep_id_t& epid) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        // Lookup the destination node address using the endpoint ID
        const auto key_node = _get_sep_node(epid);

        // Build a return val
        sep_info_t retval;
        retval.has_ctrl        = (key_node.extended_info >> 0) & 0x1;
        retval.has_data        = (key_node.extended_info >> 1) & 0x1;
        retval.num_input_ports = retval.has_data ? ((key_node.extended_info >> 2) & 0x3F)
                                                 : 0;
        retval.num_output_ports = retval.has_data ? ((key_node.extended_info >> 8) & 0x3F)
                                                  : 0;
        retval.reports_strm_errs = (key_node.extended_info >> 14) & 0x1;
        retval.addr              = {key_node.device_id, key_node.inst};
        return retval;
    }

    void setup_local_route(chdr_ctrl_xport& xport, const sep_id_t& dst_epid) override
    {
        using node_type = topo_node_t::node_type;
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid = xport.get_epid();

        auto dst_node = _get_sep_node(dst_epid);
        auto route    = _tgraph->get_route(_my_node_id, dst_node);
        UHD_LOG_TRACE(LOG_ID, "Setting up local route: " << to_string(route));
        route.pop_back();

        // Initialize all nodes between host and destination SEP. This will
        // program all nodes to do the reverse routing (how to send packets to
        // my_epid, i.e. back to the host). This requires one transaction per
        // node on the route.
        topo_edge_t last_edge;
        last_edge.dst_port    = -1;
        topo_node_t last_node = route.front().node;
        for (const auto& hop : route) {
            UHD_LOG_TRACE(LOG_ID,
                "Initializing node " << hop.node.to_string()
                                     << " to send data back to EPID " << my_epid);
            mgmt_payload init_req_xact;
            _traverse_to_node(init_req_xact, last_node, my_epid, last_edge.src_port);
            _push_node_init_hop(init_req_xact, hop.node, my_epid, last_edge.dst_port);
            // Send the transaction and receive a response.
            // We don't care about the contents of the response.
            _send_recv_mgmt_transaction(xport, init_req_xact);
            last_edge = hop.edge;
            last_node = hop.node;
        }

        // Build a management transaction to configure all the nodes in the path going to
        // dst_epid. This will program the nodes to do the forward routing (how
        // to send packets to dst_epid). Unlike the reverse routing, we can do
        // that in a single management transaction.
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        for (const auto& hop : route) {
            mgmt_hop_t curr_cfg_hop;
            switch (hop.node.type) {
                case node_type::XBAR: {
                    // Configure the routing table to route all packets going to dst_epid
                    // to the port with index next_dest_t
                    curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                        mgmt_op_t::cfg_payload(dst_epid, hop.edge.src_port)));
                    curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_SEL_DEST,
                        mgmt_op_t::sel_dest_payload(
                            static_cast<uint16_t>(hop.edge.src_port))));
                } break;
                case node_type::XPORT: {
                    const uint8_t node_subtype =
                        static_cast<uint8_t>(hop.node.extended_info & 0xFF);
                    // Run a hop configuration function for custom transports
                    if (_rtcfg_cfg_fns.count(node_subtype)) {
                        _rtcfg_cfg_fns.at(node_subtype)(hop.node.device_id,
                            hop.node.inst,
                            node_subtype,
                            curr_cfg_hop);
                    } else {
                        curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
                    }
                } break;
                case node_type::STRM_EP: {
                    // Stream endpoints are not involved in routing, so do nothing
                } break;
                default: {
                    UHD_THROW_INVALID_CODE_PATH();
                } break;
            }
            // Add this hop to the trancation only if it's not empty
            if (curr_cfg_hop.get_num_ops() > 0) {
                cfg_xact.add_hop(curr_cfg_hop);
            }
        }

        // If we follow this route then we should end up at a stream endpoint
        // so add an extra hop and return the packet back with the node info.
        // We will sanity check it later
        mgmt_hop_t discover_hop;
        discover_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_INFO_REQ));
        discover_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
        cfg_xact.add_hop(discover_hop);

        // Send the transaction and validate that we saw a stream endpoint
        const mgmt_payload sep_info_xact = _send_recv_mgmt_transaction(xport, cfg_xact);
        const topo_node_t sep_node       = _pop_node_discovery_hop(sep_info_xact);
        if (sep_node.type != topo_node_t::node_type::STRM_EP) {
            throw uhd::routing_error(
                "Route setup failed. Could not confirm terminal stream endpoint");
        }

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Established a route from EPID=%d (SW) to EPID=%d")
                % xport.get_epid() % dst_epid));
        UHD_LOG_TRACE(LOG_ID,
            (boost::format("The destination for EPID=%d has been added to all routers in "
                           "the path: %s")
                % dst_epid % to_string(route)));
    }

    bool can_remote_route(
        const sep_addr_t& dst_addr, const sep_addr_t& src_addr) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        const auto src_node = _get_sep_node(src_addr);
        const auto dst_node = _get_sep_node(dst_addr);

        return _tgraph->has_route(src_node, dst_node);
    }

    route_type get_route(const sep_addr_t& node_addr) const override
    {
        return _tgraph->get_route(_my_node_id, topo_node_t(node_addr));
    }

    void setup_remote_route(chdr_ctrl_xport& xport,
        const sep_id_t& dst_epid,
        const sep_id_t& src_epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        if (not is_endpoint_registered(dst_epid)) {
            throw uhd::routing_error("mgmt_portal::setup_remote_route(): Route setup "
                                     "failed. The requested destination endpoint "
                                     + std::to_string(dst_epid)
                                     + " was not bound to an EPID and registered");
        }
        if (not is_endpoint_registered(src_epid)) {
            throw uhd::routing_error("mgmt_portal::setup_remote_route(): Route setup "
                                     "failed. The requested source endpoint "
                                     + std::to_string(src_epid)
                                     + " was not bound to an EPID and registered");
        }

        if (!can_remote_route(
                _get_sep_node(dst_epid).get_addr(), _get_sep_node(src_epid).get_addr())) {
            const std::string err_msg = "Failed to route from EPID "
                                        + std::to_string(src_epid) + " to EPID "
                                        + std::to_string(dst_epid) + ". No route found";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        const auto my_epid  = xport.get_epid();
        const auto src_node = _get_sep_node(src_epid);
        const auto dst_node = _get_sep_node(dst_epid);
        const auto route    = _tgraph->get_route(src_node, dst_node);
        UHD_HERE();

        auto in_edge = route.cbegin()->edge;
        for (auto hop_it = route.cbegin(); hop_it != route.cend(); ++hop_it) {
            const auto node     = hop_it->node;
            const auto out_edge = hop_it->edge;
            if (node.type == topo_node_t::node_type::XBAR) {
                mgmt_payload cfg_xact;
                cfg_xact.set_header(my_epid, _protover, _chdr_w);
                _traverse_to_node(cfg_xact, node, my_epid, -1, true);
                mgmt_hop_t xbar_route_hop;
                xbar_route_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                    mgmt_op_t::cfg_payload(src_epid, in_edge.dst_port)));
                xbar_route_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                    mgmt_op_t::cfg_payload(dst_epid, out_edge.src_port)));
                xbar_route_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
                cfg_xact.add_hop(xbar_route_hop);
                _send_recv_mgmt_transaction(xport, cfg_xact);
            }

            in_edge = out_edge;
        }

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Programmed a route from EPID=%d to EPID=%s") % src_epid
                % dst_epid));
    }

    void config_local_rx_stream_start(chdr_ctrl_xport& xport,
        const sep_id_t& epid,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const stream_buff_params_t& fc_freq,
        const stream_buff_params_t& fc_headroom,
        const bool reset = false) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid = xport.get_epid();

        // The discovery process has already setup a route from the
        // destination to us. No additional action is necessary.

        auto dst_node = _get_sep_node(epid);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, dst_node, my_epid);

        mgmt_hop_t cfg_hop;
        // Assert reset if requested
        if (reset) {
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                mgmt_op_t::cfg_payload(REG_RESET_AND_FLUSH, RESET_AND_FLUSH_OSTRM)));
        }
        // Set destination of the stream to us (this endpoint)
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_DST_EPID, my_epid)));
        // Configure flow control parameters
        _push_ostrm_flow_control_config(lossy_xport,
            pyld_buff_fmt,
            mdata_buff_fmt,
            _endianness != HOST_ENDIANNESS,
            fc_freq,
            fc_headroom,
            cfg_hop);
        // Return the packet back to us
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));

        // Send the transaction and receive a response.
        // We don't care about the contents of the response.
        cfg_xact.add_hop(cfg_hop);
        _send_recv_mgmt_transaction(xport, cfg_xact);

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Initiated RX stream setup for EPID=%d") % epid));
    }

    stream_buff_params_t config_local_rx_stream_commit(chdr_ctrl_xport& xport,
        const sep_id_t& epid,
        const double timeout  = 0.2,
        const bool fc_enabled = true) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        auto dst_node = _get_sep_node(epid);

        // Wait for stream configuration to finish on the HW side
        _validate_stream_setup(xport, dst_node, timeout, fc_enabled);

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Finished RX stream setup for EPID=%d") % epid));

        // Return discovered buffer parameters
        return std::get<1>(_get_ostrm_status(xport, dst_node));
    }

    void config_local_tx_stream(chdr_ctrl_xport& xport,
        const sep_id_t& epid,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const bool reset = false) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid  = xport.get_epid();
        auto dst_node = _get_sep_node(epid);

        // First setup a route between to the endpoint
        setup_local_route(xport, epid);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, dst_node, my_epid);

        mgmt_hop_t cfg_hop;
        // Assert reset if requested
        if (reset) {
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                mgmt_op_t::cfg_payload(REG_RESET_AND_FLUSH, RESET_AND_FLUSH_ISTRM)));
        }
        // Configure buffer types
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_ISTRM_CTRL_STATUS,
                BUILD_CTRL_STATUS_WORD(false,
                    false,
                    pyld_buff_fmt,
                    mdata_buff_fmt,
                    _endianness != HOST_ENDIANNESS))));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
        cfg_xact.add_hop(cfg_hop);

        // Send the transaction and receive a response.
        // We don't care about the contents of the response.
        _send_recv_mgmt_transaction(xport, cfg_xact);

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Finished TX stream setup for EPID=%d") % epid));
    }

    stream_buff_params_t config_remote_stream(chdr_ctrl_xport& xport,
        const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const stream_buff_params_t& fc_freq,
        const stream_buff_params_t& fc_headroom,
        const bool reset     = false,
        const double timeout = 0.2) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid          = xport.get_epid();
        const bool fc_enabled = (fc_freq.bytes != 0) || (fc_freq.packets != 0);

        // First setup a route between the two endpoints
        setup_remote_route(xport, dst_epid, src_epid);

        const auto src_node = _get_sep_node(src_epid);
        const auto dst_node = _get_sep_node(dst_epid);

        // If requested, send transactions to reset and flush endpoints
        if (reset) {
            // Reset source and destination (in that order)
            for (size_t i = 0; i < 2; i++) {
                mgmt_payload rst_xact;
                rst_xact.set_header(my_epid, _protover, _chdr_w);
                _traverse_to_node(rst_xact, (i == 0) ? src_node : dst_node, my_epid);
                mgmt_hop_t rst_hop;
                rst_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                    mgmt_op_t::cfg_payload(REG_RESET_AND_FLUSH,
                        (i == 0) ? RESET_AND_FLUSH_OSTRM : RESET_AND_FLUSH_ISTRM)));
                rst_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
                rst_xact.add_hop(rst_hop);
                _send_recv_mgmt_transaction(xport, rst_xact);
            }
        }

        // Build a management transaction to configure the source node
        {
            mgmt_payload cfg_xact;
            cfg_xact.set_header(my_epid, _protover, _chdr_w);
            _traverse_to_node(cfg_xact, src_node, my_epid);
            mgmt_hop_t cfg_hop;
            // Set destination of the stream to dst_epid
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                mgmt_op_t::cfg_payload(REG_OSTRM_DST_EPID, dst_epid)));
            // Configure flow control parameters
            _push_ostrm_flow_control_config(
                lossy_xport, BUFF_U64, BUFF_U64, false, fc_freq, fc_headroom, cfg_hop);
            // Return the packet back to us
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));

            // Send the transaction and receive a response.
            // We don't care about the contents of the response.
            cfg_xact.add_hop(cfg_hop);
            _send_recv_mgmt_transaction(xport, cfg_xact);
        }

        // Build a management transaction to configure the destination node
        {
            mgmt_payload cfg_xact;
            cfg_xact.set_header(my_epid, _protover, _chdr_w);
            _traverse_to_node(cfg_xact, dst_node, my_epid);
            mgmt_hop_t cfg_hop;
            // Configure buffer types
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                mgmt_op_t::cfg_payload(REG_ISTRM_CTRL_STATUS,
                    BUILD_CTRL_STATUS_WORD(false, false, BUFF_U64, BUFF_U64, false))));
            // Return the packet back to us
            cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
            // Send the transaction and receive a response.
            // We don't care about the contents of the response.
            cfg_xact.add_hop(cfg_hop);
            _send_recv_mgmt_transaction(xport, cfg_xact);
        }

        // Wait for stream configuration to finish on the HW side
        _validate_stream_setup(xport, src_node, timeout, fc_enabled);

        UHD_LOG_DEBUG(LOG_ID,
            (boost::format("Setup a stream from EPID=%d to EPID=%d") % src_epid
                % dst_epid));

        // Return discovered buffer parameters
        return std::get<1>(_get_ostrm_status(xport, src_node));
    }


    void register_xport_hop_cfg_fns(uint8_t xport_subtype,
        xport_cfg_fn_t init_hop_cfg_fn,
        xport_cfg_fn_t rtcfg_hop_cfg_fn) override
    {
        _init_cfg_fns[xport_subtype]  = init_hop_cfg_fn;
        _rtcfg_cfg_fns[xport_subtype] = rtcfg_hop_cfg_fn;
    }


private: // Functions
    // Discover all nodes that are reachable from this software stream endpoint
    void _discover_topology(chdr_ctrl_xport& xport)
    {
        using port_t    = topo_edge_t::port_t;
        using node_type = topo_node_t::node_type;
        // Initialize a queue of pending paths. We will use this for a breadth-first
        // traversal of the dataflow graph. The queue consists of a previously discovered
        // node and the next destination to take from that node.
        std::queue<std::pair<topo_node_t, port_t>> pending_paths;
        const auto my_epid = xport.get_epid();

        // Add ourselves to the the pending queue to kick off the search
        UHD_LOG_DEBUG(LOG_ID,
            "Starting topology discovery from " << _my_node_id.to_string());
        pending_paths.push({_my_node_id, port_t(-1)});

        while (!pending_paths.empty()) {
            // Pop the next path to discover from the pending queue
            const auto next_path = pending_paths.front();
            pending_paths.pop();
            const auto& next_node = next_path.first;

            // Build a management transaction to first get to our destination so that we
            // can ask it to identify itself
            mgmt_payload route_xact;
            route_xact.set_header(my_epid, _protover, _chdr_w);
            _traverse_to_node(route_xact, next_node, my_epid, next_path.second);

            // Discover downstream node (we ask the node to identify itself)
            mgmt_payload disc_req_xact(route_xact);
            // Push a node discovery hop
            mgmt_hop_t disc_hop;
            disc_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_INFO_REQ));
            disc_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
            disc_req_xact.add_hop(disc_hop);
            UHD_LOG_TRACE(LOG_ID,
                "Discovering next node upstream from "
                    << next_node.to_string() << ", output port " << next_path.second
                    << " using management transaction: " << disc_req_xact.to_string());

            topo_node_t new_node;
            try {
                // Send the discovery transaction
                const mgmt_payload disc_resp_xact =
                    _send_recv_mgmt_transaction(xport, disc_req_xact);
                new_node = _pop_node_discovery_hop(disc_resp_xact);
            } catch (uhd::io_error& io_err) {
                // We received an IO error. This could happen if we have a legitimate
                // error or if there is no node to discover downstream. We can't tell for
                // sure why but we can guess. If the next_path for this node is -1 then we
                // expect something to be here, in which case we treat this as a
                // legitimate error. In all other cases we assume that there was nothing
                // to discover downstream.
                if (next_path.second < 0) {
                    UHD_LOG_ERROR(LOG_ID, io_err.what());
                    throw io_err;
                } else {
                    // Move to the next pending path
                    UHD_LOG_TRACE(LOG_ID,
                        "Nothing connected on " << next_path.first.to_string() << "->"
                                                << next_path.second
                                                << ". Ignoring that path.");
                    continue;
                }
            }

            // We found a node!
            // Identify destination port number. This only matters on a crossbar.
            // Crossbars publish the port we're connected on in the 'inst' field.
            // However, that means we need to manually count the instances of
            // crossbars to uniquely identify them.
            port_t dst_port = topo_edge_t::ANY_PORT;
            if (new_node.type == node_type::XBAR) {
                dst_port = new_node.inst;
                // TODO: For now, we assume one xbar per device, so this is easy.
                // If we want to allow multiple crossbars, we either need to add
                // some identification, or we identify crossbars by the things
                // they're connected to (e.g., only one crossbar can be attached
                // to next_node on this very port).
                new_node.inst = 0;
            }

            topo_edge_t new_edge(next_node, new_node, next_path.second, dst_port);

            // Because next_node was unknown before topo discovery started, we
            // can safely add a route from next_node to new_node. That doesn't
            // preclude that new_node was previously detected, so we check for that.
            // If new_node was already in the graph, we can terminate the discovery
            // on this path.
            if (!_tgraph->add_biedge(next_node, new_node, new_edge)) {
                UHD_LOG_DEBUG(LOG_ID,
                    "Re-discovered node " << new_node.to_string() << ". Skipping it");
                continue;
            }
            UHD_LOG_DEBUG(LOG_ID, "Discovered node " << new_node.to_string());

            // Initialize the node (first time config)
            mgmt_payload init_req_xact(route_xact);
            _push_node_init_hop(init_req_xact, new_node, my_epid, new_edge.dst_port);
            // Send the transaction and receive a response.
            // We don't care about the contents of the response.
            _send_recv_mgmt_transaction(xport, init_req_xact);
            UHD_LOG_DEBUG(LOG_ID, "Initialized node " << new_node.to_string());

            // If the new node is a stream endpoint then we are done traversing this
            // path. If not, then check all ports downstream of the new node and add
            // them to pending_paths for further traversal
            switch (new_node.type) {
                case node_type::XBAR: {
                    // Total ports on this crossbar
                    const size_t nports =
                        static_cast<size_t>(new_node.extended_info & 0xFF);
                    // Total transport ports on this crossbar (the first nports_xport
                    // ports are transport ports)
                    const size_t nports_xport =
                        static_cast<size_t>((new_node.extended_info >> 8) & 0xFF);
                    // When we allow daisy chaining, we need to recursively check
                    // other transports
                    const size_t start_port = ALLOW_DAISY_CHAINING ? 0 : nports_xport;
                    for (size_t i = start_port; i < nports; i++) {
                        // Skip the input port
                        if (i != static_cast<size_t>(dst_port)) {
                            pending_paths.push(
                                std::make_pair(new_node, static_cast<port_t>(i)));
                        }
                    }
                    UHD_LOG_TRACE(LOG_ID,
                        "* " << new_node.to_string() << " has " << nports << " ports, "
                             << nports_xport
                             << " transports and we are hooked up on port " << dst_port);
                } break;
                case node_type::STRM_EP: {
                    // Stop searching when we find a stream endpoint
                } break;
                case node_type::XPORT: {
                    // A transport has only one output. We don't need to take
                    // any action to reach the next node.
                    pending_paths.push(std::make_pair(new_node, -1));
                } break;
                default: {
                    UHD_THROW_INVALID_CODE_PATH();
                    break;
                }
            }
            // That's it! Now go check the next path.
        }
    }

    //! Add hops to the management transaction to reach the specified node
    //
    // The typical use case is to add a management transaction that's aimed at
    // the node after \p dst_node. If the intention is to add a management
    // transaction for \p dst_node itself, then set \p dst_is_target to true.
    topo_edge_t _traverse_to_node(mgmt_payload& transaction,
        const topo_node_t& dst_node,
        const sep_id_t my_epid,
        const topo_edge_t::port_t last_port_override = -1,
        const bool dst_is_target                     = false)
    {
        if (dst_node == _my_node_id) {
            return topo_edge_t();
        }
        auto route = _tgraph->get_route(_my_node_id, dst_node);
        if (dst_is_target) {
            route.pop_back();
        }
        route.back().edge.src_port = last_port_override;
        topo_edge_t last_edge;
        for (const auto& hop : route) {
            mgmt_hop_t mgmt_hop;
            switch (hop.node.type) {
                case topo_node_t::node_type::STRM_EP:
                    // This is a stream endpoint. Nothing needs to be done to
                    // advance here. The behavior of this operation is identical
                    // whether or not the stream endpoint is in software or not.
                    continue;
                case topo_node_t::node_type::XBAR: {
                    // If we're on a crossbar, then we have to manually choose
                    // how to get to the next node. We also program the return
                    // path.
                    UHD_ASSERT_THROW(hop.edge.src_port >= 0);
                    UHD_ASSERT_THROW(last_edge.dst_port >= 0);
                    mgmt_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                        mgmt_op_t::cfg_payload(my_epid, last_edge.dst_port)));
                    mgmt_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_SEL_DEST,
                        mgmt_op_t::sel_dest_payload(
                            static_cast<uint16_t>(hop.edge.src_port))));
                } break;
                default:
                    // For any other node, we only need a NOP to traverse.
                    mgmt_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
                    break;
            }
            transaction.add_hop(mgmt_hop);
            last_edge = hop.edge;
        }
        return route.back().edge;
    }

    // Add operations to a hop to configure flow control for an output stream
    void _push_ostrm_flow_control_config(const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const bool byte_swap,
        const stream_buff_params_t& fc_freq,
        const stream_buff_params_t& fc_headroom,
        mgmt_hop_t& hop)
    {
        // Validate flow control parameters
        if (fc_freq.bytes > MAX_FC_FREQ_BYTES || fc_freq.packets > MAX_FC_FREQ_PKTS) {
            throw uhd::value_error("Flow control frequency parameters out of bounds");
        }
        if (fc_headroom.bytes > MAX_FC_HEADROOM_BYTES
            || fc_headroom.packets > MAX_FC_HEADROOM_PKTS) {
            throw uhd::value_error("Flow control headroom parameters out of bounds");
        }

        // Add flow control parameters to hop
        hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_FC_FREQ_BYTES_LO,
                static_cast<uint32_t>(fc_freq.bytes & uint64_t(0xFFFFFFFF)))));
        hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(
                REG_OSTRM_FC_FREQ_BYTES_HI, static_cast<uint32_t>(fc_freq.bytes >> 32))));
        hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(
                REG_OSTRM_FC_FREQ_PKTS, static_cast<uint32_t>(fc_freq.packets))));
        const uint32_t headroom_reg =
            (static_cast<uint32_t>(fc_headroom.bytes) & 0xFFFF)
            | ((static_cast<uint32_t>(fc_headroom.packets) & 0xFF) << 16);
        hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_FC_HEADROOM, headroom_reg)));
        // Configure buffer types and lossy_xport, then start configuration
        hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_CTRL_STATUS,
                BUILD_CTRL_STATUS_WORD(
                    true, lossy_xport, pyld_buff_fmt, mdata_buff_fmt, byte_swap))));
    }

    // Send/recv a management transaction that will get the output stream status
    std::tuple<uint32_t, stream_buff_params_t> _get_ostrm_status(
        chdr_ctrl_xport& xport, const topo_node_t& dst)
    {
        auto my_epid = xport.get_epid();
        // Build a management transaction to first get to the node
        mgmt_payload status_xact;
        status_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(status_xact, dst, my_epid);

        // Read all the status registers
        mgmt_hop_t cfg_hop;
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_RD_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_CTRL_STATUS)));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_RD_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_BUFF_CAP_BYTES_LO)));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_RD_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_BUFF_CAP_BYTES_HI)));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_RD_REQ,
            mgmt_op_t::cfg_payload(REG_OSTRM_BUFF_CAP_PKTS)));
        cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
        status_xact.add_hop(cfg_hop);

        // Send the transaction, receive a response and validate it
        const mgmt_payload resp_xact = _send_recv_mgmt_transaction(xport, status_xact);
        if (resp_xact.get_num_hops() != 1) {
            throw uhd::op_failed("Management operation failed. Incorrect format (hops).");
        }
        const mgmt_hop_t& rhop = resp_xact.get_hop(0);
        if (rhop.get_num_ops() < 2
            || rhop.get_op(0).get_op_code() != mgmt_op_t::MGMT_OP_NOP) {
            throw uhd::op_failed(
                "Management operation failed. Incorrect format (operations).");
        }
        for (size_t i = 1; i < rhop.get_num_ops(); i++) {
            if (rhop.get_op(i).get_op_code() != mgmt_op_t::MGMT_OP_CFG_RD_RESP) {
                throw uhd::op_failed(
                    "Management operation failed. Incorrect format (operations).");
            }
        }

        // Extract peek data from transaction
        mgmt_op_t::cfg_payload status_pl    = rhop.get_op(1).get_op_payload();
        mgmt_op_t::cfg_payload cap_bytes_lo = rhop.get_op(2).get_op_payload();
        mgmt_op_t::cfg_payload cap_bytes_hi = rhop.get_op(3).get_op_payload();
        mgmt_op_t::cfg_payload cap_pkts     = rhop.get_op(4).get_op_payload();

        stream_buff_params_t buff_params;
        buff_params.bytes = static_cast<uint64_t>(cap_bytes_lo.data)
                            | (static_cast<uint64_t>(cap_bytes_hi.data) << 32);
        buff_params.packets = static_cast<uint32_t>(cap_pkts.data);
        return std::make_tuple(status_pl.data, buff_params);
    }

    // Make sure that stream setup is complete and successful, else throw exception
    void _validate_stream_setup(chdr_ctrl_xport& xport,
        const topo_node_t& dst_node,
        const double timeout,
        const bool fc_enabled)
    {
        // Get the status of the output stream
        uint32_t ostrm_status = 0;
        double sleep_s        = 0.001;
        for (size_t i = 0; i < size_t(std::ceil(timeout / sleep_s)); i++) {
            ostrm_status = std::get<0>(_get_ostrm_status(xport, dst_node));
            if ((ostrm_status & STRM_STATUS_SETUP_PENDING) != 0) {
                // Wait and retry
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int64_t>(sleep_s * 1000)));
            } else {
                // Configuration is done
                break;
            }
        }

        if ((ostrm_status & STRM_STATUS_SETUP_PENDING) != 0) {
            throw uhd::op_timeout("config_stream: Operation timed out");
        }
        if ((ostrm_status & STRM_STATUS_SETUP_ERR) != 0) {
            throw uhd::op_failed("config_stream: Setup failure");
        }
        if (fc_enabled != bool(ostrm_status & STRM_STATUS_FC_ENABLED)) {
            throw uhd::op_failed("config_stream: Flow control negotiation failed");
        }
    }


    // Pop a node discovery response from a transaction and parse it
    const topo_node_t _pop_node_discovery_hop(const mgmt_payload& transaction)
    {
        if (transaction.get_num_hops() != 1) {
            throw uhd::op_failed("Management operation failed. Incorrect format (hops).");
        }
        const mgmt_hop_t& rhop = transaction.get_hop(0);
        if (rhop.get_num_ops() < 2) {
            throw uhd::op_failed(
                "Management operation failed. Incorrect number of operations.");
        }
        const mgmt_op_t& nop_resp  = rhop.get_op(0);
        const mgmt_op_t& info_resp = rhop.get_op(1);
        if (nop_resp.get_op_code() != mgmt_op_t::MGMT_OP_NOP
            || info_resp.get_op_code() != mgmt_op_t::MGMT_OP_INFO_RESP) {
            throw uhd::op_failed(
                "Management operation failed. Incorrect format (operations).");
        }
        mgmt_op_t::node_info_payload resp_pl(info_resp.get_op_payload());
        return topo_node_t(resp_pl.device_id,
            static_cast<topo_node_t::node_type>(resp_pl.node_type),
            resp_pl.node_inst,
            resp_pl.ext_info);
    }

    // Push a hop onto a transaction to initialize the current node
    void _push_node_init_hop(mgmt_payload& transaction,
        const topo_node_t& node,
        const sep_id_t& my_epid,
        const topo_edge_t::port_t dst_port)
    {
        mgmt_hop_t init_hop;
        switch (node.type) {
            case topo_node_t::node_type::XBAR: {
                // Configure the routing table to route all packets going to my_epid back
                // to the port where the packet is entering
                // The address for the transaction is the EPID and the data is the port #
                init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                    mgmt_op_t::cfg_payload(my_epid, dst_port)));
            } break;
            case topo_node_t::node_type::STRM_EP: {
                // Do nothing
                init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
            } break;
            case topo_node_t::node_type::XPORT: {
                uint8_t node_subtype = static_cast<uint8_t>(node.extended_info & 0xFF);
                // Run a hop configuration function for custom transports
                if (_init_cfg_fns.count(node_subtype)) {
                    _init_cfg_fns.at(node_subtype)(
                        node.device_id, node.inst, node_subtype, init_hop);
                } else {
                    // For a generic transport, just advertise the transaction to the
                    // outside world. The generic xport adapter will do the rest
                    init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_ADVERTISE));
                }
            } break;
            default: {
                UHD_THROW_INVALID_CODE_PATH();
            } break;
        }
        init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
        transaction.add_hop(init_hop);
    }

    //! Returns the node object for a given stream endpoint ID (virtual or non-virtual)
    topo_node_t _get_sep_node(const sep_id_t& epid) const
    {
        // Lookup the destination node address using the endpoint ID
        auto nodes = _tgraph->get_nodes([epid](const topo_node_t& node) {
            return (node.type == topo_node_t::node_type::STRM_EP
                       || node.type == topo_node_t::node_type::VIRTUAL)
                   && node.epid == epid;
        });
        if (nodes.size() > 1) {
            throw uhd::lookup_error("mgmt_portal::_get_sep_node(): Found more than one ("
                                    + std::to_string(nodes.size())
                                    + ") stream endpoint with EPID "
                                    + std::to_string(epid));
        }
        if (nodes.empty()) {
            throw uhd::lookup_error("mgmt_portal::_get_sep_node(): Could not find a "
                                    "stream endpoint with specified EPID "
                                    + std::to_string(epid));
        }

        return nodes.front();
    }

    topo_node_t _get_sep_node(const sep_addr_t& addr) const
    {
        // Lookup the destination node address using the address
        auto nodes = _tgraph->get_nodes([addr](const topo_node_t& node) {
            return (node.type == topo_node_t::node_type::STRM_EP
                       || node.type == topo_node_t::node_type::VIRTUAL)
                   && node.device_id == addr.first && node.inst == addr.second;
        });
        if (nodes.size() > 1) {
            throw uhd::lookup_error(
                "mgmt_portal::_get_sep_node(): Found more than one ("
                + std::to_string(nodes.size()) + ") stream endpoint with address "
                + std::to_string(addr.first) + ":" + std::to_string(addr.second));
        }
        if (nodes.empty()) {
            throw uhd::lookup_error("mgmt_portal::_get_sep_node(): Could not find a "
                                    "stream endpoint with address "
                                    + std::to_string(addr.first) + ":"
                                    + std::to_string(addr.second));
        }

        return nodes.front();
    }


    // Send the specified management transaction to the device
    void _send_mgmt_transaction(
        chdr_ctrl_xport& xport, const mgmt_payload& payload, double timeout = 0.1)
    {
        chdr_header header;
        header.set_pkt_type(PKT_TYPE_MGMT);
        header.set_num_mdata(0);
        header.set_seq_num(static_cast<uint16_t>(_send_seqnum++));
        header.set_length(uhd::narrow_cast<uint16_t>(
            payload.get_size_bytes() + (chdr_w_to_bits(_chdr_w) / 8)));
        header.set_dst_epid(0);

        auto send_buff = xport.get_send_buff(static_cast<int32_t>(timeout * 1000));
        if (not send_buff) {
            UHD_LOG_ERROR(
                LOG_ID, "Timed out getting send buff for management transaction");
            throw uhd::io_error("Timed out getting send buff for management transaction");
        }
        _send_pkt->refresh(send_buff->data(), header, payload);
        send_buff->set_packet_size(header.get_length());
        xport.release_send_buff(std::move(send_buff));
    }

    // Send the specified management transaction to the device and receive a response
    const mgmt_payload _send_recv_mgmt_transaction(
        chdr_ctrl_xport& xport, const mgmt_payload& transaction, double timeout = 0.1)
    {
        auto my_epid = xport.get_epid();
        mgmt_payload send(transaction);
        send.set_header(my_epid, _protover, _chdr_w);
        // If we are expecting to receive a response then we have to add an additional
        // NO-OP hop for the receive endpoint. All responses will be appended to this hop.
        mgmt_hop_t nop_hop;
        nop_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
        send.add_hop(nop_hop);
        // Send the transaction over the wire
        _send_mgmt_transaction(xport, send);

        auto mgmt_buff = xport.get_mgmt_buff(static_cast<int32_t>(timeout * 1000));
        if (not mgmt_buff) {
            throw uhd::io_error("Timed out getting recv buff for management transaction");
        }
        _recv_pkt->refresh(mgmt_buff->data());
        mgmt_payload recv;
        recv.set_header(my_epid, _protover, _chdr_w);
        _recv_pkt->fill_payload(recv);
        xport.release_mgmt_buff(std::move(mgmt_buff));
        return recv;
    }

private: // Members
    // The software RFNoC protocol version
    const uint16_t _protover;
    // CHDR Width for this design/application
    const chdr_w_t _chdr_w;
    // Endianness for the transport
    const endianness_t _endianness;
    // The node ID for this software endpoint
    const topo_node_t _my_node_id;
    // Send/recv transports
    size_t _send_seqnum = 0;
    // Management packet containers
    chdr_mgmt_packet::uptr _send_pkt;
    chdr_mgmt_packet::cuptr _recv_pkt;
    // Hop configuration function maps
    std::map<uint8_t, xport_cfg_fn_t> _init_cfg_fns;
    std::map<uint8_t, xport_cfg_fn_t> _rtcfg_cfg_fns;
    // Reference to the topology graph
    topo_graph_t::sptr _tgraph;
    // Mutex that protects all state in this class
    mutable std::recursive_mutex _mutex;
}; // class mgmt_portal_impl


mgmt_portal::uptr mgmt_portal::make(chdr_ctrl_xport& xport,
    const chdr::chdr_packet_factory& pkt_factory,
    sep_addr_t my_sep_addr,
    uhd::rfnoc::detail::topo_graph_t::sptr topo_graph)
{
    return std::make_unique<mgmt_portal_impl>(
        xport, pkt_factory, my_sep_addr, topo_graph);
}

}}} // namespace uhd::rfnoc::mgmt
