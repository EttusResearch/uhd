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
constexpr uint16_t REG_ISTRM_CTRL_STATUS       = 0x38; // RW

constexpr uint32_t RESET_AND_FLUSH_OSTRM = (1 << 0);
constexpr uint32_t RESET_AND_FLUSH_ISTRM = (1 << 1);
// constexpr uint32_t RESET_AND_FLUSH_CTRL  = (1 << 2);
constexpr uint32_t RESET_AND_FLUSH_ALL   = 0x7;

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


//! The type of a node in the data-flow graph
enum class node_type {
    //! Invalid type. The FPGA will never have a node with type = 0
    NODE_TYPE_INVALID = 0,
    //! CHDR Crossbar
    NODE_TYPE_XBAR = 1,
    //! Stream Endpoint
    NODE_TYPE_STRM_EP = 2,
    //! Transport
    NODE_TYPE_XPORT = 3
};

//! A unique identifier for a node
struct node_id_t
{
    //! A unique ID for device that houses this node
    device_id_t device_id = NULL_DEVICE_ID;
    //! The type of this node
    node_type type = node_type::NODE_TYPE_INVALID;
    //! The instance number of this node in the device
    sep_inst_t inst = 0;
    //! Extended info about node (not used for comparisons)
    // The value depends on the node type. For example, this includes number of
    // ports on a crossbar, data/ctrl capability for SEPs, or transport subtype
    // for transport adapters.
    // It contains up to 18 bits of information.
    uint32_t extended_info = 0;

    // ctors and operators
    node_id_t()                     = default;
    node_id_t(const node_id_t& rhs) = default;
    node_id_t(device_id_t device_id_, node_type type_, sep_inst_t inst_)
        : device_id(device_id_), type(type_), inst(inst_), extended_info(0)
    {
    }
    node_id_t(device_id_t device_id_,
        node_type type_,
        sep_inst_t inst_,
        uint32_t extended_info_)
        : device_id(device_id_), type(type_), inst(inst_), extended_info(extended_info_)
    {
    }
    node_id_t(const sep_addr_t& sep_addr)
        : device_id(sep_addr.first)
        , type(node_type::NODE_TYPE_STRM_EP)
        , inst(sep_addr.second)
        , extended_info(0)
    {
    }

    inline uint64_t unique_id() const
    {
        return (static_cast<uint64_t>(inst) + (static_cast<uint64_t>(device_id) << 16)
                + (static_cast<uint64_t>(type) << 32));
    }
    inline std::string to_string() const
    {
        static const std::map<node_type, std::string> NODE_STR = {
            {node_type::NODE_TYPE_INVALID, "unknown"},
            {node_type::NODE_TYPE_XBAR, "xbar"},
            {node_type::NODE_TYPE_STRM_EP, "sep"},
            {node_type::NODE_TYPE_XPORT, "xport"}};
        return str(
            boost::format("device:%d/%s:%d") % device_id % NODE_STR.at(type) % inst);
    }

    inline friend bool operator<(const node_id_t& lhs, const node_id_t& rhs)
    {
        return (lhs.unique_id() < rhs.unique_id());
    }
    inline friend bool operator==(const node_id_t& lhs, const node_id_t& rhs)
    {
        return (lhs.unique_id() == rhs.unique_id());
    }
    inline friend bool operator!=(const node_id_t& lhs, const node_id_t& rhs)
    {
        return (lhs.unique_id() != rhs.unique_id());
    }
    inline node_id_t& operator=(const node_id_t&) = default;
};

//! The local destination to take at the current node to reach the next node
//  - If negative, then no specific action necessary
//  - If non-negative, then route (select destination) to the value
using next_dest_t = int32_t;

//! An address that allows locating a node in a data-flow network starting from
//  a specific stream endpoint. The address is a collection (vector) of nodes and
//  the respective routing decisions to get to the final node.
using node_addr_t = std::vector<std::pair<node_id_t, next_dest_t>>;

std::string to_string(const node_addr_t& node_addr)
{
    if (!node_addr.empty()) {
        std::string str("");
        for (const auto& hop : node_addr) {
            str += hop.first.to_string() + std::string(",") + std::to_string(hop.second)
                   + std::string("->");
        }
        return str;
    } else {
        return std::string("<empty>");
    }
}

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
        sep_addr_t my_sep_addr)
        : _protover(pkt_factory.get_protover())
        , _chdr_w(pkt_factory.get_chdr_w())
        , _endianness(pkt_factory.get_endianness())
        , _my_node_id(my_sep_addr.first, node_type::NODE_TYPE_STRM_EP, xport.get_epid())
        , _send_seqnum(0)
        , _send_pkt(pkt_factory.make_mgmt())
        , _recv_pkt(pkt_factory.make_mgmt())
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _discover_topology(xport);
        UHD_LOG_DEBUG("RFNOC::MGMT",
            "The following endpoints are reachable from " << _my_node_id.to_string());
        for (const auto& ep : _discovered_ep_set) {
            UHD_LOG_DEBUG("RFNOC::MGMT", "* " << ep.first << ":" << ep.second);
        }
    }

    ~mgmt_portal_impl() override {}

    const std::set<sep_addr_t>& get_reachable_endpoints() const override
    {
        return _discovered_ep_set;
    }

    void initialize_endpoint(
        chdr_ctrl_xport& xport, const sep_addr_t& addr, const sep_id_t& epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        auto my_epid = xport.get_epid();

        // Create a node ID from lookup info
        node_id_t lookup_node(addr.first, node_type::NODE_TYPE_STRM_EP, addr.second);
        if (_node_addr_map.count(lookup_node) == 0) {
            throw uhd::lookup_error(
                "initialize_endpoint(): Cannot reach node with specified address.");
        }
        const node_addr_t& node_addr = _node_addr_map.at(lookup_node);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, node_addr);

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
        node_id_t lookup_node(addr.first, node_type::NODE_TYPE_STRM_EP, addr.second);
        if (_node_addr_map.count(lookup_node) == 0) {
            throw uhd::lookup_error(
                "initialize_endpoint(): Cannot reach node with specified address.");
        }
        // Add/update the entry in the stream endpoint ID map
        _epid_addr_map[epid] = addr;
        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format("Bound stream endpoint with Addr=(%d,%d) to EPID=%d")
                % addr.first % addr.second % epid));
        UHD_LOG_TRACE("RFNOC::MGMT",
            (boost::format(
                 "Stream endpoint with EPID=%d can be reached by taking the path: %s")
                % epid % to_string(_node_addr_map.at(lookup_node))));
    }

    bool is_endpoint_registered(const sep_id_t& epid) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        return (_epid_addr_map.count(epid) > 0);
    }

    sep_info_t get_endpoint_info(const sep_id_t& epid) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        // Lookup the destination node address using the endpoint ID
        if (_epid_addr_map.count(epid) == 0) {
            throw uhd::lookup_error(
                "get_endpoint_info(): Could not find a stream with specified ID.");
        }
        node_id_t lookup_node(_epid_addr_map.at(epid));
        // If a node is in _epid_addr_map then it must be in _node_addr_map
        UHD_ASSERT_THROW(_node_addr_map.count(lookup_node) > 0);
        // Why is key_node different from lookup_node?
        // Because it has additional extended info (look at operator< def)
        const node_id_t& key_node = _node_addr_map.find(lookup_node)->first;

        // Build a return val
        sep_info_t retval;
        retval.has_ctrl        = (key_node.extended_info >> 0) & 0x1;
        retval.has_data        = (key_node.extended_info >> 1) & 0x1;
        retval.num_input_ports = retval.has_data ? ((key_node.extended_info >> 2) & 0x3F)
                                                 : 0;
        retval.num_output_ports = retval.has_data ? ((key_node.extended_info >> 8) & 0x3F)
                                                  : 0;
        retval.reports_strm_errs = (key_node.extended_info >> 14) & 0x1;
        retval.addr              = _epid_addr_map.at(epid);
        return retval;
    }

    void setup_local_route(chdr_ctrl_xport& xport, const sep_id_t& dst_epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid = xport.get_epid();

        // Look up the physical stream endpoint address using the endpoint ID
        // node_addr contains the route to the host SEP to the destination SEP
        const node_addr_t& node_addr = _lookup_sep_node_addr(dst_epid);

        // Initialize all nodes between host and destination SEP. This will
        // program all nodes to do the reverse routing (how to send packets to
        // my_epid, i.e. back to the host).
        node_addr_t route_addr = node_addr_t();
        route_addr.push_back(std::make_pair(_my_node_id, next_dest_t(-1)));
        for (const auto& addr_pair : node_addr) {
            mgmt_payload init_req_xact;
            _traverse_to_node(init_req_xact, route_addr);
            _push_node_init_hop(init_req_xact, addr_pair.first, my_epid);
            // Send the transaction and receive a response.
            // We don't care about the contents of the response.
            _send_recv_mgmt_transaction(xport, init_req_xact);
            route_addr.push_back(addr_pair);
        }

        // Build a management transaction to configure all the nodes in the path going to
        // dst_epid
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);

        for (const auto& addr_pair : node_addr) {
            const node_id_t& curr_node   = addr_pair.first;
            const next_dest_t& curr_dest = addr_pair.second;
            mgmt_hop_t curr_cfg_hop;
            switch (curr_node.type) {
                case node_type::NODE_TYPE_XBAR: {
                    // Configure the routing table to route all packets going to dst_epid
                    // to the port with index next_dest_t
                    curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                        mgmt_op_t::cfg_payload(dst_epid, curr_dest)));
                    curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_SEL_DEST,
                        mgmt_op_t::sel_dest_payload(static_cast<uint16_t>(curr_dest))));
                } break;
                case node_type::NODE_TYPE_XPORT: {
                    uint8_t node_subtype =
                        static_cast<uint8_t>(curr_node.extended_info & 0xFF);
                    // Run a hop configuration function for custom transports
                    if (_rtcfg_cfg_fns.count(node_subtype)) {
                        _rtcfg_cfg_fns.at(node_subtype)(curr_node.device_id,
                            curr_node.inst,
                            node_subtype,
                            curr_cfg_hop);
                    } else {
                        curr_cfg_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
                    }
                } break;
                case node_type::NODE_TYPE_STRM_EP: {
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
        const node_id_t sep_node         = _pop_node_discovery_hop(sep_info_xact);
        if (sep_node.type != node_type::NODE_TYPE_STRM_EP) {
            throw uhd::routing_error(
                "Route setup failed. Could not confirm terminal stream endpoint");
        }

        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format("Established a route from EPID=%d (SW) to EPID=%d")
                % xport.get_epid() % dst_epid));
        UHD_LOG_TRACE("RFNOC::MGMT",
            (boost::format("The destination for EPID=%d has been added to all routers in "
                           "the path: %s")
                % dst_epid % to_string(node_addr)));
    }

    bool can_remote_route(
        const sep_addr_t& dst_addr, const sep_addr_t& src_addr) const override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        if ((_discovered_ep_set.count(dst_addr) == 0)
            || (_discovered_ep_set.count(src_addr) == 0)) {
            // Can't route to/from something if we don't know about it
            return false;
        }

        UHD_ASSERT_THROW(_node_addr_map.count(node_id_t(dst_addr)) > 0);
        UHD_ASSERT_THROW(_node_addr_map.count(node_id_t(src_addr)) > 0);

        // Lookup the src and dst node address using the endpoint ID
        const node_addr_t& dst_node_addr = _node_addr_map.at(node_id_t(dst_addr));
        const node_addr_t& src_node_addr = _node_addr_map.at(node_id_t(src_addr));

        // Find a common parent (could be faster than n^2 but meh, this is easier)
        // Note: This is *not* finding the fastest path from dst_addr to src_addr.
        // This using the existing routes we have, and finding a route through
        // a common parent that also needs to be a crossbar.
        for (const auto& dnode : dst_node_addr) {
            for (const auto& snode : src_node_addr) {
                if (dnode.first == snode.first
                    && dnode.first.type == node_type::NODE_TYPE_XBAR) {
                    return true;
                }
            }
        }
        return false;
    }

    void setup_remote_route(chdr_ctrl_xport& xport,
        const sep_id_t& dst_epid,
        const sep_id_t& src_epid) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        if (not is_endpoint_registered(dst_epid)) {
            throw uhd::routing_error("Route setup failed. The destination endpoint was "
                                     "not bound to an EPID and registered");
        }
        if (not is_endpoint_registered(src_epid)) {
            throw uhd::routing_error("Route setup failed. The source endpoint was "
                                     "not bound to an EPID and registered");
        }

        if (not can_remote_route(
                _epid_addr_map.at(dst_epid), _epid_addr_map.at(src_epid))) {
            throw uhd::routing_error("Route setup failed. The endpoints don't share a "
                                     "common crossbar parent.");
        }

        // If we setup local routes from this host to both the source and destination
        // endpoints then the routing algorithm will guarantee that packet between src and
        // dst will have a path between them as long as they share a common parent
        // (crossbar). The assumption is verified above. It is also guaranteed that the
        // path between them will be the shortest one. It is possible that we are
        // configuring more crossbars than necessary but we do this for simplicity. If
        // there is a need to optimize for routing table fullness, we can do a software
        // graph traversal here, find the closest common parent (crossbar) for the two
        // nodes and only configure the nodes downstream of that.
        setup_local_route(xport, dst_epid);
        setup_local_route(xport, src_epid);

        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format(
                 "The two routes above now enable a route from EPID=%d to EPID=%s")
                % src_epid % dst_epid));
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

        const node_addr_t& node_addr = _lookup_sep_node_addr(epid);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, node_addr);

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

        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format("Initiated RX stream setup for EPID=%d") % epid));
    }

    stream_buff_params_t config_local_rx_stream_commit(chdr_ctrl_xport& xport,
        const sep_id_t& epid,
        const double timeout  = 0.2,
        const bool fc_enabled = true) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        // Wait for stream configuration to finish on the HW side
        const node_addr_t& node_addr = _lookup_sep_node_addr(epid);
        _validate_stream_setup(xport, node_addr, timeout, fc_enabled);

        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format("Finished RX stream setup for EPID=%d") % epid));

        // Return discovered buffer parameters
        return std::get<1>(_get_ostrm_status(xport, node_addr));
    }

    void config_local_tx_stream(chdr_ctrl_xport& xport,
        const sep_id_t& epid,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const bool reset = false) override
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto my_epid = xport.get_epid();

        // First setup a route between to the endpoint
        setup_local_route(xport, epid);

        const node_addr_t& node_addr = _lookup_sep_node_addr(epid);

        // Build a management transaction to first get to the node
        mgmt_payload cfg_xact;
        cfg_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(cfg_xact, node_addr);

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

        UHD_LOG_DEBUG("RFNOC::MGMT",
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
        auto my_epid = xport.get_epid();
        const bool fc_enabled = (fc_freq.bytes != 0) || (fc_freq.packets != 0);

        // First setup a route between the two endpoints
        setup_remote_route(xport, dst_epid, src_epid);

        const node_addr_t& dst_node_addr = _lookup_sep_node_addr(dst_epid);
        const node_addr_t& src_node_addr = _lookup_sep_node_addr(src_epid);

        // If requested, send transactions to reset and flush endpoints
        if (reset) {
            // Reset source and destination (in that order)
            for (size_t i = 0; i < 2; i++) {
                mgmt_payload rst_xact;
                rst_xact.set_header(my_epid, _protover, _chdr_w);
                _traverse_to_node(rst_xact, (i == 0) ? src_node_addr : dst_node_addr);
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
            _traverse_to_node(cfg_xact, src_node_addr);
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
            _traverse_to_node(cfg_xact, dst_node_addr);
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
        _validate_stream_setup(xport, src_node_addr, timeout, fc_enabled);

        UHD_LOG_DEBUG("RFNOC::MGMT",
            (boost::format("Setup a stream from EPID=%d to EPID=%d") % src_epid
                % dst_epid));

        // Return discovered buffer parameters
        return std::get<1>(_get_ostrm_status(xport, src_node_addr));
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
        // Initialize a queue of pending paths. We will use this for a breadth-first
        // traversal of the dataflow graph. The queue consists of a previously discovered
        // node and the next destination to take from that node.
        std::queue<std::pair<node_id_t, next_dest_t>> pending_paths;
        auto my_epid = xport.get_epid();

        // Add ourselves to the the pending queue to kick off the search
        UHD_LOG_DEBUG("RFNOC::MGMT",
            "Starting topology discovery from " << _my_node_id.to_string());
        bool is_first_path = true;
        pending_paths.push(std::make_pair(_my_node_id, next_dest_t(-1)));

        while (not pending_paths.empty()) {
            // Pop the next path to discover from the pending queue
            const auto& next_path = pending_paths.front();
            pending_paths.pop();

            // We need to build a node_addr_t to allow us to get to next_path
            // To do so we first lookup how to get to next_path.first. This location has
            // already been discovered so we should just be able to look it up in
            // _node_addr_map. The only exception for that is when we are just starting
            // out, in which case our previous node is "us".
            node_addr_t next_addr = is_first_path ? node_addr_t()
                                                  : _node_addr_map.at(next_path.first);
            // Once we know how to get to the base node, then add the next destination
            next_addr.push_back(next_path);
            is_first_path = false;

            // Build a management transaction to first get to our destination so that we
            // can ask it to identify itself
            mgmt_payload route_xact;
            route_xact.set_header(my_epid, _protover, _chdr_w);
            _traverse_to_node(route_xact, next_addr);

            // Discover downstream node (we ask the node to identify itself)
            mgmt_payload disc_req_xact(route_xact);
            // Push a node discovery hop
            mgmt_hop_t disc_hop;
            disc_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_INFO_REQ));
            disc_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_RETURN));
            disc_req_xact.add_hop(disc_hop);

            node_id_t new_node;
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
                    throw io_err;
                } else {
                    // Move to the next pending path
                    UHD_LOG_TRACE("RFNOC::MGMT",
                        "Nothing connected on " << next_path.first.to_string() << "->"
                                                << next_path.second
                                                << ". Ignoring that path.");
                    continue;
                }
            }

            // We found a node!
            // First check if we have already seen this node in the past. If not, we have
            // to add it to our internal data structures. If we have already seen it then
            // we just skip it. It is OK to skip the node because we are doing a BFS,
            // which means that the first time a node is discovered during the traversal,
            // the distance from this EP to that node will be the shortest path. The core
            // design philosophy for RFNoC is that the data will always take the shortest
            // path, because we make the assumption that a shorter path *always* has
            // better QoS compared to a longer one. If this assumption is not true, we
            // have to handle ordering by QoS for which we need to modify this search a
            // bit and provide QoS preferences in the API. That may be a future feature.
            if (_node_addr_map.count(new_node) > 0) {
                UHD_LOG_DEBUG("RFNOC::MGMT",
                    "Re-discovered node " << new_node.to_string() << ". Skipping it");
            } else {
                UHD_LOG_DEBUG("RFNOC::MGMT", "Discovered node " << new_node.to_string());
                _node_addr_map[new_node] = next_addr;

                // Initialize the node (first time config)
                mgmt_payload init_req_xact(route_xact);
                _push_node_init_hop(init_req_xact, new_node, my_epid);
                // Send the transaction and receive a response.
                // We don't care about the contents of the response.
                _send_recv_mgmt_transaction(xport, init_req_xact);
                UHD_LOG_DEBUG("RFNOC::MGMT", "Initialized node " << new_node.to_string());

                // If the new node is a stream endpoint then we are done traversing this
                // path. If not, then check all ports downstream of the new node and add
                // them to pending_paths for further traversal
                switch (new_node.type) {
                    case node_type::NODE_TYPE_XBAR: {
                        // Total ports on this crossbar
                        size_t nports =
                            static_cast<size_t>(new_node.extended_info & 0xFF);
                        // Total transport ports on this crossbar (the first nports_xport
                        // ports are transport ports)
                        size_t nports_xport =
                            static_cast<size_t>((new_node.extended_info >> 8) & 0xFF);
                        // When we allow daisy chaining, we need to recursively check
                        // other transports
                        size_t start_port = ALLOW_DAISY_CHAINING ? 0 : nports_xport;
                        for (size_t i = start_port; i < nports; i++) {
                            // Skip the current port because it's the input
                            if (i != static_cast<size_t>(new_node.inst)) {
                                // If there is a single downstream port then do nothing
                                pending_paths.push(std::make_pair(
                                    new_node, static_cast<next_dest_t>(i)));
                            }
                        }
                        UHD_LOG_TRACE("RFNOC::MGMT",
                            "* " << new_node.to_string() << " has " << nports
                                 << " ports, " << nports_xport
                                 << " transports and we are hooked up on port "
                                 << new_node.inst);
                    } break;
                    case node_type::NODE_TYPE_STRM_EP: {
                        // Stop searching when we find a stream endpoint
                        // Add the endpoint to the discovered endpoint vector
                        _discovered_ep_set.insert(
                            sep_addr_t(new_node.device_id, new_node.inst));
                    } break;
                    case node_type::NODE_TYPE_XPORT: {
                        // A transport has only one output. We don't need to take
                        // any action to reach
                        pending_paths.push(std::make_pair(new_node, -1));
                    } break;
                    default: {
                        UHD_THROW_INVALID_CODE_PATH();
                        break;
                    }
                }
            }
        }
    }

    // Add hops to the management transaction to reach the specified node
    void _traverse_to_node(mgmt_payload& transaction, const node_addr_t& node_addr)
    {
        for (const auto& addr_pair : node_addr) {
            const node_id_t& curr_node   = addr_pair.first;
            const next_dest_t& curr_dest = addr_pair.second;
            if (curr_node.type != node_type::NODE_TYPE_STRM_EP) {
                // If a node is a crossbar, then it must have a non-negative destination
                UHD_ASSERT_THROW(
                    (curr_node.type != node_type::NODE_TYPE_XBAR || curr_dest >= 0));
                _push_advance_hop(transaction, curr_dest);
            } else {
                // This is a stream endpoint. Nothing needs to be done to advance
                // here. The behavior of this operation is identical whether or
                // not the stream endpoint is in software or not.
            }
        }
    }

    // Add a hop to the transaction simply to get to the next node
    void _push_advance_hop(mgmt_payload& transaction, const next_dest_t& next_dst)
    {
        if (next_dst >= 0) {
            mgmt_hop_t sel_dest_hop;
            sel_dest_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_SEL_DEST,
                mgmt_op_t::sel_dest_payload(static_cast<uint16_t>(next_dst))));
            transaction.add_hop(sel_dest_hop);
        } else {
            mgmt_hop_t nop_hop;
            nop_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
            transaction.add_hop(nop_hop);
        }
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
        chdr_ctrl_xport& xport, const node_addr_t& node_addr)
    {
        auto my_epid = xport.get_epid();
        // Build a management transaction to first get to the node
        mgmt_payload status_xact;
        status_xact.set_header(my_epid, _protover, _chdr_w);
        _traverse_to_node(status_xact, node_addr);

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
        const node_addr_t& node_addr,
        const double timeout,
        const bool fc_enabled)
    {
        // Get the status of the output stream
        uint32_t ostrm_status = 0;
        double sleep_s        = 0.001;
        for (size_t i = 0; i < size_t(std::ceil(timeout / sleep_s)); i++) {
            ostrm_status = std::get<0>(_get_ostrm_status(xport, node_addr));
            if ((ostrm_status & STRM_STATUS_SETUP_PENDING) != 0) {
                // Wait and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(sleep_s * 1000)));
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
    const node_id_t _pop_node_discovery_hop(const mgmt_payload& transaction)
    {
        if (transaction.get_num_hops() != 1) {
            throw uhd::op_failed("Management operation failed. Incorrect format (hops).");
        }
        const mgmt_hop_t& rhop     = transaction.get_hop(0);
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
        return std::move(node_id_t(resp_pl.device_id,
            static_cast<node_type>(resp_pl.node_type),
            resp_pl.node_inst,
            resp_pl.ext_info));
    }

    // Push a hop onto a transaction to initialize the current node
    void _push_node_init_hop(
        mgmt_payload& transaction, const node_id_t& node, const sep_id_t& my_epid)
    {
        mgmt_hop_t init_hop;
        switch (node.type) {
            case node_type::NODE_TYPE_XBAR: {
                // Configure the routing table to route all packets going to my_epid back
                // to the port where the packet is entering
                // The address for the transaction is the EPID and the data is the port #
                init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_CFG_WR_REQ,
                    mgmt_op_t::cfg_payload(my_epid, node.inst)));
            } break;
            case node_type::NODE_TYPE_STRM_EP: {
                // Do nothing
                init_hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP));
            } break;
            case node_type::NODE_TYPE_XPORT: {
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

    // Lookup the full address of a stream endpoint node given the EPID
    const node_addr_t& _lookup_sep_node_addr(const sep_id_t& epid)
    {
        // Lookup the destination node address using the endpoint ID
        if (_epid_addr_map.count(epid) == 0) {
            throw uhd::lookup_error(
                "Could not find a stream endpoint with the requested ID.");
        }
        node_id_t sep_node(_epid_addr_map.at(epid));
        // If a node is in _epid_addr_map then it must be in _node_addr_map
        UHD_ASSERT_THROW(_node_addr_map.count(sep_node) > 0);
        return _node_addr_map.at(sep_node);
    }

    // Send the specified management transaction to the device
    void _send_mgmt_transaction(
        chdr_ctrl_xport& xport, const mgmt_payload& payload, double timeout = 0.1)
    {
        chdr_header header;
        header.set_pkt_type(PKT_TYPE_MGMT);
        header.set_num_mdata(0);
        header.set_seq_num(static_cast<uint16_t>(_send_seqnum++));
        header.set_length(uhd::narrow_cast<uint16_t>(payload.get_size_bytes() + (chdr_w_to_bits(_chdr_w) / 8)));
        header.set_dst_epid(0);

        auto send_buff = xport.get_send_buff(static_cast<int32_t>(timeout * 1000));
        if (not send_buff) {
            UHD_LOG_ERROR(
                "RFNOC::MGMT", "Timed out getting send buff for management transaction");
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
    const node_id_t _my_node_id;
    // A table that maps a node_id_t to a node_addr_t. This map allows looking up the
    // address of a node given the node ID. There may be multiple ways to get to the
    // node but we only store the shortest path here.
    std::map<node_id_t, node_addr_t> _node_addr_map;
    // A list of all discovered endpoints
    std::set<sep_addr_t> _discovered_ep_set;
    // A table that maps a stream endpoint ID to the physical address of the stream
    // endpoint. This is a cache of the values from the epid_allocator
    std::map<sep_id_t, sep_addr_t> _epid_addr_map;
    // Send/recv transports
    size_t _send_seqnum;
    // Management packet containers
    chdr_mgmt_packet::uptr _send_pkt;
    chdr_mgmt_packet::cuptr _recv_pkt;
    // Hop configuration function maps
    std::map<uint8_t, xport_cfg_fn_t> _init_cfg_fns;
    std::map<uint8_t, xport_cfg_fn_t> _rtcfg_cfg_fns;
    // Mutex that protects all state in this class
    mutable std::recursive_mutex _mutex;
}; // namespace mgmt


mgmt_portal::uptr mgmt_portal::make(chdr_ctrl_xport& xport,
    const chdr::chdr_packet_factory& pkt_factory,
    sep_addr_t my_sep_addr)
{
    return std::make_unique<mgmt_portal_impl>(xport, pkt_factory, my_sep_addr);
}

}}} // namespace uhd::rfnoc::mgmt
