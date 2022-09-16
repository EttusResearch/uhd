//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <boost/format.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/optional.hpp>
#include <list>
#include <map>
#include <memory>

namespace uhd { namespace rfnoc { namespace detail {


/*! Representation of a node in an RFNoC topology graph
 *
 * Nodes in such a graph include stream endpoints, transport adapters, crossbars,
 * etc. (see also topo_node_t::node_type).
 */
struct topo_node_t
{
    /*** Types ***************************************************************/
    using node_hash_type = uint64_t;

    //! The type of a node in the data-flow graph
    //
    // Note: The values for the enum entries must match with those in
    // rfnoc_chdr_internal_utils.vh.
    enum class node_type {
        //! Invalid type. The FPGA will never have a node with type = 0
        INVALID = 0,
        //! CHDR Crossbar
        XBAR = 1,
        //! Stream Endpoint
        STRM_EP = 2,
        //! Transport
        XPORT = 3,
        //! A virtual endpoint is a non-UHD owned endpoint off-device. This has
        // no correspondence in the FPGA, so we just use a big value for this.
        VIRTUAL = 100
    };


    /*** Constructors ********************************************************/
    topo_node_t()                       = default;
    topo_node_t(const topo_node_t& rhs) = default;
    topo_node_t(device_id_t device_id_,
        node_type type_,
        sep_inst_t inst_,
        uint32_t extended_info_               = 0,
        const boost::optional<sep_id_t> epid_ = boost::none)
        : device_id(device_id_)
        , type(type_)
        , inst(inst_)
        , extended_info(extended_info_)
        , epid(epid_)
    {
    }
    topo_node_t(const sep_addr_t& sep_addr,
        const bool is_local_                  = false,
        const boost::optional<sep_id_t> epid_ = boost::none)
        : device_id(sep_addr.first)
        , type(node_type::STRM_EP)
        , inst(sep_addr.second)
        , extended_info(0)
        , epid(epid_)
        , is_local_sep(is_local_)
    {
    }

    /*** Attributes **********************************************************/
    //! A unique ID for device that houses this node
    device_id_t device_id = NULL_DEVICE_ID;
    //! The type of this node
    node_type type = node_type::INVALID;
    //! The instance number of this node in the device
    sep_inst_t inst = 0;
    //! Extended info about node (not used for comparisons)
    // The value depends on the node type. For example, this includes number of
    // ports on a crossbar, data/ctrl capability for SEPs, or transport subtype
    // for transport adapters.
    // It contains up to 18 bits of information.
    uint32_t extended_info = 0;

    //! If applicable, the endpoint ID of this node. This is only set if
    // type == node_type::STRM_EP or node_type::VIRTUAL.
    boost::optional<sep_id_t> epid;

    //! True if this is a stream endpoint (type == node_type::STRM_EP) and the
    // endpoint is local, i.e. based within the UHD session.
    bool is_local_sep = false;

    /*** Ops and methods *****************************************************/
    std::string to_string() const
    {
        static const std::map<node_type, std::string> NODE_STR = {
            {node_type::INVALID, "unknown"},
            {node_type::XBAR, "xbar"},
            {node_type::STRM_EP, "sep"},
            {node_type::XPORT, "xport"},
            {node_type::VIRTUAL, "virtual"}};
        return str(boost::format("device%s:%d/%s:%d") % (is_local_sep ? "[local]" : "")
                   % device_id % NODE_STR.at(type) % inst);
    }

    // This is effectively a hashing function for this struct, used for comparisons.
    node_hash_type unique_id() const
    {
        return (static_cast<uint64_t>(inst) + (static_cast<uint64_t>(device_id) << 16)
                + (static_cast<uint64_t>(type) << 32));
    }

    bool operator==(const topo_node_t& rhs) const
    {
        return this->device_id != NULL_DEVICE_ID && this->unique_id() == rhs.unique_id();
    }

    bool operator!=(const topo_node_t& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const topo_node_t& rhs) const
    {
        return this->unique_id() < rhs.unique_id();
    }

    sep_addr_t get_addr() const
    {
        return {device_id, inst};
    }
};


/*! Representation of an edge in a topology graph
 */
struct topo_edge_t
{
    //! The type of a edge in the data-flow graph
    enum class edge_type {
        //! Invalid (used for uninitialized edges)
        INVALID,
        //! End of route: This is not a valid route, but terminates a route
        END_OF_ROUTE,
        //! Host connection: Meaning this connection goes from a Transport
        // Adapter (on the device) to an SEP (within the UHD context) or vice-versa
        HOST,
        //! On-chip: Direct AXIS connection between two nodes (e.g. between
        // SEP and crossbar)
        ON_CHIP,
        //! Off-chip: Direct AXIS connection between two nodes on two different
        // FPGAs (e.g., via Aurora connection between two crossbars).
        OFF_CHIP,
        //! Ethernet connection: This means the connection leaves the device
        // over an Ethernet transport adapter, and is routed to another device
        // via external network (could also be point-to-point Ethernet
        // connection). Such a connection requires additional routing information
        // to function.
        ETHERNET
    };

    //! Type used for indexing port numbers
    using port_t = int32_t;

    static constexpr port_t ANY_PORT = -1;

    /*** Constructors ********************************************************/
    topo_edge_t()                       = default;
    topo_edge_t(const topo_edge_t& rhs) = default;
    topo_edge_t(const topo_node_t& src,
        const topo_node_t& dst,
        const port_t src_port,
        const port_t dst_port)
        : src_port(src_port), dst_port(dst_port)
    {
        using node_type = topo_node_t::node_type;
        if (!src.is_local_sep && (src.device_id == dst.device_id)) {
            type = edge_type::ON_CHIP;
        } else if (src.is_local_sep || dst.is_local_sep) {
            type = edge_type::HOST;
        } else if (src.type == node_type::XBAR && dst.type == node_type::XBAR
                   && (src.device_id != dst.device_id)) {
            type = edge_type::OFF_CHIP;
        } else if (src.type == node_type::XPORT && dst.type == node_type::XPORT
                   && (src.device_id != dst.device_id)) {
            type = edge_type::ETHERNET;
        }
    }

    /*** Attributes **********************************************************/
    edge_type type = edge_type::INVALID;

    // Source port for this edge. This value only matters for edges from crossbars,
    // on other nodes, it may be negative (meaning: Don't care).
    port_t src_port = ANY_PORT;
    port_t dst_port = ANY_PORT;

    // This can be extended to also store the current occupancy of this link,
    // or latency, or whatever else we care about in the future. This would allow
    // us, e.g., to find the lowest-latency link between two endpoints.
    int weight = 1;
    int get_weight() const
    {
        return type == edge_type::END_OF_ROUTE ? 0 : weight;
    }
};

struct route_element_type
{
    topo_node_t node;
    topo_edge_t edge;
};

using route_type = std::list<route_element_type>;

//! Pretty-print a route
std::string to_string(const route_type& route);

//! A graph object to store the topology of our RFNoC system. Unlike
// rfnoc::detail::graph_t, this doesn't store info about the RFNoC blocks, but
// about everything else: Transport Adapters, SEPs, crossbars, and how they're
// connected.
class topo_graph_t
{
public:
    using sptr             = std::shared_ptr<topo_graph_t>;
    using node_filter_type = std::function<bool(topo_node_t&)>;

    topo_graph_t() = default;

    //! Add an unconnected node into this graph
    //
    // This is typically called while starting a new graph (or sub-graph).
    //
    // \returns true if the node was added, false if it already existed.
    bool add_node(const topo_node_t& node);

    //! Return a list of nodes that match a given predicate
    std::list<topo_node_t> get_nodes(node_filter_type&& filter_predicate) const;

    //! Return a list of nodes that are connected to a source node and optionally
    // match a given predicate.
    //
    // The source node is not included in the result.
    std::list<topo_node_t> get_connected_nodes(
        const topo_node_t& src, node_filter_type&& filter_predicate = nullptr) const;

    //! Add another connection/edge into this topology graph
    //
    // This adds another route from \p src to \p dst into this graph.
    // If dst is not yet registered, it will add it as well.
    //
    // If \p src does not exist, this will throw an exception.
    //
    // \throws uhd::runtime_error if \p src does not exist.
    // \returns true if dst was newly added, false if it already existed.
    bool add_edge(
        const topo_node_t& src, const topo_node_t& dst, const topo_edge_t& edge);

    //! Add a bidirectional edge into this topology
    //
    // This is mostly identical to add_edge(), but it adds a reverse edge as
    // well as a forward edge. Functionally, it is equivalent to this pseudo
    // code:
    //
    // ~~~{.cpp}
    // add_edge(src, dst, edge);
    // swap(edge.src_port, edge.dst_port);
    // add_route(dst, src, edge);
    // ~~~
    //
    // \throws uhd::runtime_error if \p src does not exist.
    // \returns true if dst was newly added, false if it already existed.
    bool add_biedge(
        const topo_node_t& src, const topo_node_t& dst, const topo_edge_t& edge);

    //! Returns true if there's a route from \p src to \p dst
    bool has_route(const topo_node_t& src, const topo_node_t& dst) const;

    //! Returns the shortest route from \p src to \p dst
    //
    // If there are multiple shortest routes, it returns the first route it finds.
    route_type get_route(const topo_node_t& src, const topo_node_t& dst) const;

    //! Returns the shortest route to \p dst from any source that matches a predicate
    //
    // Use this instead of get_route() if it is not clear which node to use as
    // source node.
    //
    // If there are multiple shortest routes, it returns the first route it finds.
    route_type get_best_route(
        node_filter_type&& src_filter, const topo_node_t& dst) const;

    //! Returns the distance between \p src and \p dst along the
    // shortest path.
    int get_distance(const topo_node_t& src, const topo_node_t& dst) const;

    //! Returns a copy of an edge object
    topo_edge_t get_edge(const topo_node_t& src, const topo_node_t& dst) const;

    //! Return a read/write reference to a node object
    topo_node_t& access_node(const topo_node_t& node_id);

    //! Update the weight (aka distance) on a specific edge
    void update_weight(
        const topo_node_t& src, const topo_edge_t::port_t src_port, const int new_weight);

    //! Dump the entire graph as DOT code to stdout
    std::string to_dot() const;


private:
    /**************************************************************************
     * Graph-related types
     *************************************************************************/
    // Naming conventions:
    // - 'vertex' and 'node' are generally synonymous in a graph context, but
    //   we'll use 'vertex' for BGL related items, and 'node' for RFNoC nodes
    // - We may use CamelCase occasionally if it fits the BGL examples and/or
    //   reference designs, in case someone needs to learn BGL to understand
    //   this code

    struct vertex_property_t
    {
        enum { num = 5000 };
        typedef boost::vertex_property_tag kind;
    };
    using TopoVertexProperty = boost::property<vertex_property_t, topo_node_t>;

    struct edge_property_t
    {
        int weight;
        size_t edge_index;
    };

    /*! The type of the BGL graph we're using
     *
     * - It is bidirectional because we need to access both in_edges and
     *   out_edges
     * - All container types are according to the BGL manual recommendations for
     *   this kind of graph
     */
    using topo_adjlist_t = boost::adjacency_list<boost::vecS,
        boost::vecS,
        boost::bidirectionalS,
        TopoVertexProperty,
        edge_property_t>;

    using node_map_t =
        std::map<topo_node_t::node_hash_type, topo_adjlist_t::vertex_descriptor>;

    class node_finder_visitor;
    class node_storer_visitor;

    /**************************************************************************
     * Private graph helpers
     *************************************************************************/
    bool _has_node(const topo_node_t& node) const
    {
        return _node_map.count(node.unique_id());
    }

    void _assert_route(
        const topo_node_t& src, const topo_node_t& dst, const std::string& verb) const;

    //! Returns a list of all vertices that match a given filter condition
    std::list<topo_adjlist_t::vertex_descriptor> _get_vertices(
        node_filter_type&& filter) const;

    /**************************************************************************
     * Attributes
     *************************************************************************/
    topo_adjlist_t _graph;

    node_map_t _node_map;

    std::vector<topo_edge_t> _edge_info;
};

}}} // namespace uhd::rfnoc::detail
