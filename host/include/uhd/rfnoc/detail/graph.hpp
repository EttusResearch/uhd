//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc/node.hpp>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace detail {

//! Container for the logical graph within an uhd::rfnoc_graph
class UHD_API graph_t
{
public:
    using uptr = std::unique_ptr<graph_t>;
    //! A shorthand for a pointer to a node
    using node_ref_t = uhd::rfnoc::node_t*;
    //! Shorthand to existing graph_edge_t
    using graph_edge_t = uhd::rfnoc::graph_edge_t;

    graph_t();
    ~graph_t();

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

    /*! Remove a connection from the graph
     *
     * After this function returns, the nodes will be considered disconnected
     * along the ports specified in \p edge_info.
     *
     * \param src_node A reference to the source node
     * \param dst_node A reference to the destination node
     * \param edge_info Information about the type of edge
     */
    void disconnect(node_ref_t src_node, node_ref_t dst_node, graph_edge_t edge_info);

    /*! Remove a node from the graph
     *
     * Disconnects all edges and removes the node from the graph.
     *
     * \param node A reference to the node
     */
    void remove(node_ref_t node);

    /*! Commit graph and run initial checks
     *
     * This method can be called anytime, but it's intended to be called when
     * the graph has been committed. It will run checks on the graph and run a
     * property propagation.
     *
     * \throws uhd::resolve_error if the properties fail to resolve.
     */
    void commit();

    /*! Opposite of commit()
     *
     * Calling this will disable property propagation until commit() has been
     * called an equal number of times.
     */
    void release();

    /*! Shutdown graph: Permenanently release
     *
     * This will release the graph permanently and safely. All ongoing property
     * and action handling is completed and then disabled (this means that
     * calling shutdown while blocks are still working will cause actions to not
     * get delivered).
     */
    void shutdown();

    /*! Return a list of all edges
     */
    std::vector<graph_edge_t> enumerate_edges();

    /*! Create a dot representation of the current graph
     *
     * The graph is represented in the dot language, which can be visualized
     * using the Graphviz tools. It contains all blocks and their connections.
     * The connections are drawn between the ports of the blocks.
     */
    std::string to_dot();

private:
    friend class graph_accessor_t;
    struct impl;
    std::unique_ptr<impl> _impl;
};


}}} /* namespace uhd::rfnoc::detail */
