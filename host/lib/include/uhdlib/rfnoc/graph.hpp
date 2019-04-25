//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_GRAPH_HPP
#define INCLUDED_LIBUHD_GRAPH_HPP

#include <uhd/rfnoc/node.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <tuple>
#include <memory>

namespace uhd { namespace rfnoc {

/*! A container that holds information about a graph edge
 */
struct graph_edge_t
{
    enum edge_t {
        STATIC, ///< A static connection between two blocks in the FPGA
        DYNAMIC, ///< A user (dynamic) connection between two blocks in the FPGA
        RX_STREAM, ///< A connection from an FPGA block to a software RX streamer
        TX_STREAM ///< A connection from a software TX streamer and an FPGA block
    };

    graph_edge_t() = default;

    graph_edge_t(const size_t src_port_,
        const size_t dst_port_,
        const edge_t edge_,
        const bool ppa)
        : src_port(src_port_)
        , dst_port(dst_port_)
        , edge(edge_)
        , property_propagation_active(ppa)
    {
    }

    //! The block ID of the source block for this edge
    std::string src_blockid;
    //! The port number of the source block for this edge
    size_t src_port = 0;
    //! The block ID of the destination block for this edge
    std::string dst_blockid;
    //! The port number of the destination block for this edge
    size_t dst_port = 0;
    //! The type of edge
    edge_t edge = DYNAMIC;
    //! When true, the framework will use this edge for property propagation
    bool property_propagation_active = true;

    bool operator==(const graph_edge_t& rhs) const
    {
        return std::tie(src_blockid,
                   src_port,
                   dst_blockid,
                   dst_port,
                   edge,
                   property_propagation_active)
               == std::tie(rhs.src_blockid,
                      rhs.src_port,
                      rhs.dst_blockid,
                      rhs.dst_port,
                      rhs.edge,
                      rhs.property_propagation_active);
    }
};


namespace detail {

//! Container for the logical graph within an uhd::rfnoc_graph
class graph_t
{
public:
    using uptr = std::unique_ptr<graph_t>;
    //! A shorthand for a pointer to a node
    using node_ref_t = uhd::rfnoc::node_t*;

    using graph_edge_t = uhd::rfnoc::graph_edge_t;

    /*! Add a connection to the graph
     *
     * After this function returns, the nodes will be considered connected
     * along the ports specified in \p edge_info.
     *
     * \param src_node A reference to the source node
     * \param dst_node A reference to the destination node
     * \param edge_info Information about the type of edge
     */
    void connect(node_ref_t src_node, node_ref_t dst_node, graph_edge_t edge_info);

    //void disconnect(node_ref_t src_node,
        //node_ref_t dst_node,
        //const size_t src_port,
        //const size_t dst_port);
        //

    /*! Run initial checks for graph
     *
     * This method can be called anytime, but it's intended to be called when
     * the graph has been committed. It will run checks on the graph and run a
     * property propagation.
     *
     * \throws uhd::resolve_error if the properties fail to resolve.
     */
    void initialize();


private:
    friend class graph_accessor_t;

    /**************************************************************************
     * Graph-related types
     *************************************************************************/
    // Naming conventions:
    // - 'vertex' and 'node' are generally ambiguous in a graph context, but
    //   we'll use vertex for BGL related items, and node for RFNoC nodes
    // - We may use CamelCase occasionally if it fits the BGL examples and/or
    //   reference designs, in case someone needs to learn BGL to understand
    //   this code

    struct vertex_property_t
    {
        enum { num = 4000 };
        typedef boost::vertex_property_tag kind;
    };
    using RfnocVertexProperty = boost::property<vertex_property_t, node_ref_t>;

    struct edge_property_t
    {
        enum { num = 4001 };
        typedef boost::edge_property_tag kind;
    };
    using RfnocEdgeProperty = boost::property<edge_property_t, graph_edge_t>;

    /*! The type of the BGL graph we're using
     *
     * - It is bidirectional because we need to access both in_edges and
     *   out_edges
     * - All container types are according to the BGL manual recommendations for
     *   this kind of graph
     */
    using rfnoc_graph_t = boost::adjacency_list<boost::vecS,
        boost::vecS,
        boost::bidirectionalS,
        RfnocVertexProperty,
        RfnocEdgeProperty>;

    using vertex_list_t = std::list<rfnoc_graph_t::vertex_descriptor>;

    template <bool forward_edges_only = true>
    struct ForwardBackwardEdgePredicate
    {
        ForwardBackwardEdgePredicate() {} // Default ctor is required
        ForwardBackwardEdgePredicate(rfnoc_graph_t& graph) : _graph(&graph) {}

        template <typename Edge>
        bool operator()(const Edge& e) const
        {
            graph_edge_t edge_info = boost::get(edge_property_t(), *_graph, e);
            return edge_info.property_propagation_active == forward_edges_only;
        }

    private:
        // Don't make any attribute const, because default assignment operator
        // is also required
        rfnoc_graph_t* _graph;
    };

    using ForwardEdgePredicate = ForwardBackwardEdgePredicate<true>;
    using BackEdgePredicate = ForwardBackwardEdgePredicate<false>;

    //! Vertex predicate, only selects nodes with dirty props
    struct DirtyNodePredicate;

    //! Vertex predicate, returns specific existing nodes
    struct FindNodePredicate;

    /**************************************************************************
     * Other private types
     *************************************************************************/
    using node_map_t = std::map<node_ref_t, rfnoc_graph_t::vertex_descriptor>;

    /**************************************************************************
     * The Algorithm
     *************************************************************************/
    /*! Implementation of the property propagation algorithm
     */
    void resolve_all_properties();

    /**************************************************************************
     * Private graph helpers
     *************************************************************************/
    template <typename VertexContainerType>
    std::vector<node_ref_t> _vertices_to_nodes(VertexContainerType&& vertex_container)
    {
        std::vector<node_ref_t> result{};
        result.reserve(vertex_container.size());
        for (const auto& vertex_descriptor : vertex_container) {
            result.push_back(boost::get(vertex_property_t(), _graph, vertex_descriptor));
        }
        return result;
    }

    /*! Returns a list of all nodes that have dirty properties.
     */
    vertex_list_t _find_dirty_nodes();

    /*! Returns nodes in topologically sorted order
     *
     *
     * \throws uhd::runtime_error if the graph was not sortable
     */
    vertex_list_t _get_topo_sorted_nodes();

    /*! Add a node, but only if it's not already in the graph.
     *
     * If it's already there, do nothing.
     */
    void _add_node(node_ref_t node);

    /*! Find the neighbouring node for \p origin based on \p port_info
     *
     * This function will check port_info to identify the port number and the
     * direction (input or output) from \p port_info. It will then return a
     * reference to the node that is attached to the node \p origin if such a
     * node exists, and the edge info.
     *
     * If port_info.type == res_source_info::INPUT_EDGE, then port_info.instance
     * will equal the return value's dst_port value.
     *
     * \returns A valid reference to the neighbouring node, or nullptr if no
     *          such node exists, and the corresponding edge info.
     */
    std::pair<node_ref_t, graph_edge_t> _find_neighbour(
        rfnoc_graph_t::vertex_descriptor origin, res_source_info port_info);

    /*! Forward all edge properties from this node (\p origin) to the
     * neighbouring ones
     *
     */
    void _forward_edge_props(rfnoc_graph_t::vertex_descriptor origin);

    /*! Check that the edge properties on both sides of the edge are equal
     *
     * \returns false if edge properties are not consistent
     */
    bool _assert_edge_props_consistent(rfnoc_graph_t::edge_descriptor edge);

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Storage for the actual graph
    rfnoc_graph_t _graph;

    //! Map to do a lookup node_ref_t -> vertex descriptor.
    //
    // This is technically redundant, but helps us check quickly and easily if
    // a node is already in the graph, and to yank out the appropriate node
    // descriptor without having to traverse the graph. The rfnoc_graph_t is not
    // efficient for lookups of vertices.
    node_map_t _node_map;
};


}}} /* namespace uhd::rfnoc::detail */

#endif /* INCLUDED_LIBUHD_GRAPH_HPP */
