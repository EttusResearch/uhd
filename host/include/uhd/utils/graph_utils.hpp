//
// Copyright 2019 Ettus Research, a National Instruments Branch
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <vector>


namespace uhd { namespace rfnoc {

//! Tuple that stores a block ID, as well as an optional port number
using block_port_def = std::tuple<std::string, boost::optional<size_t>>;

// TODO: Get rid of magic strings
/*! List of blocks that can terminate chains. Note that some blocks only terminate at
 *  some of their ports, so we can optionally include a port number.
 */
static const std::vector<block_port_def> TERMINATOR_BLOCKS{
    block_port_def{NODE_ID_SEP, boost::none},
    block_port_def{"Radio", boost::none},
    block_port_def{"NullSrcSink", 0}};

/*!
 *  Get a chain of blocks that statically connect back to a terminating block. This
 *  vector's first element is `start_block`, and the chain continues from there.
 *
 *  This function does not make the connections between blocks, it simply traverses the
 *  static connections.
 *
 *  \param graph The rfnoc_graph that is being examined
 *  \param start_block The block we begin to build the chain from
 *  \param port The block port of `src_port` that the path will begin at
 *  \param source_chain Whether or not the `start_block` is a source (or a destination).
 *                      If true, the chain will start at `start_block`'s output port. If
 *                      false, the chain will start with `start_block`'s input port.
 *  \return The edge list representing the data path requested
 */
std::vector<graph_edge_t> UHD_API get_block_chain(const rfnoc_graph::sptr graph,
    const block_id_t start_block,
    const size_t port,
    const bool source_chain);


/*! Connect desired blocks by whatever path that can be found
 *
 * This will find the most direct path from a source block to a destination
 * block. If these blocks are statically connected it will simply call connect()
 * on all intermediate connections. If not, it will create a dynamic connection
 * between stream endpoints. If this is not possible, an exception is thrown.
 *
 *  \param graph The rfnoc_graph that is being examined
 *  \param src_blk Source block's ID
 *  \param src_port Block port where the path starts
 *  \param dst_blk Destination block's ID
 *  \param dst_port Block port where the path ends
 *  \param skip_property_propagation Declare back-edge
 *                                   (see also uhd::rfnoc::rfnoc_graph::connect())
 *                                   If true, it will declare only the first
 *                                   connection in this chain as a back-edge.
 *
 *  \return The edge list representing the data path requested
 */
std::vector<graph_edge_t> UHD_API connect_through_blocks(rfnoc_graph::sptr graph,
    const block_id_t src_blk,
    const size_t src_port,
    const block_id_t dst_blk,
    const size_t dst_port,
    const bool skip_property_propagation = false);

}} // namespace uhd::rfnoc
