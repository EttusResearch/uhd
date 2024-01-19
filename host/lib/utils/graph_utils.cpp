//
// Copyright 2019 Ettus Research, a National Instruments Branch
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <numeric>
#include <utility>


namespace uhd { namespace rfnoc {


//! Returns whether or not a block (and port) is know to to terminate data paths
bool check_terminator_block(const block_id_t blk_id, const size_t port)
{
    const std::string blk_id_str = blk_id.get_block_name();
    for (auto term_block : TERMINATOR_BLOCKS) {
        auto optional_port = std::get<1>(term_block);
        if (blk_id_str == std::get<0>(term_block)
            && (!optional_port || port == optional_port.get())) {
            return true;
        }
    }
    return false;
}


std::vector<graph_edge_t> get_block_chain(const rfnoc_graph::sptr graph,
    const block_id_t start_block,
    const size_t port,
    const bool source_chain)
{
    // Enumerate blocks in the chain
    auto edges = graph->enumerate_static_connections();

    std::vector<graph_edge_t> block_chain;
    std::string current_block = start_block.to_string();
    size_t current_port       = port;
    while (true) {
        UHD_LOG_TRACE("GRAPH_UTILS",
            "Looking for current block " << current_block << ", port " << current_port);
        bool next_found = false;
        for (auto& edge : edges) {
            if ((source_chain)
                    ? (edge.src_blockid == current_block && edge.src_port == current_port)
                    : (edge.dst_blockid == current_block
                        && edge.dst_port == current_port)) {
                // If the current block is the edge's source, make the edge's
                // destination the current block
                next_found = true;
                UHD_LOG_TRACE(
                    "GRAPH_UTILS", " --> Found next block: " + edge.dst_blockid);

                block_chain.push_back(edge);
                current_block = (source_chain) ? edge.dst_blockid : edge.src_blockid;
                current_port  = (source_chain) ? edge.dst_port : edge.src_port;
                // Compare our current block and port
                if (check_terminator_block(current_block, current_port)) {
                    // If we've found a terminating block, stop iterating through the
                    // edges
                    break;
                }
            }
        }
        if (not next_found) {
            UHD_LOG_TRACE(
                "GRAPH_UTILS", "Failed to find current block in static connections");
            break;
        }
        if (check_terminator_block(current_block, current_port)) {
            // If we've found a terminating block, stop iterating through the edges
            break;
        }
    }
    return block_chain;
}


std::vector<graph_edge_t> connect_through_blocks(rfnoc_graph::sptr graph,
    const block_id_t src_blk,
    const size_t src_port,
    const block_id_t dst_blk,
    const size_t dst_port,
    const bool skip_property_propagation)
{
    // First, create a chain from the source block to a stream endpoint
    auto block_chain = get_block_chain(graph, src_blk, src_port, true);
    UHD_LOG_TRACE("GRAPH_UTILS", "Found source chain for " + src_blk.to_string());
    // See if dst_blk is in our block_chain already
    const bool dst_found = std::accumulate(block_chain.begin(),
        block_chain.end(),
        false,
        [dst_blk, dst_port](bool dst_found, const graph_edge_t edge) {
            // This is our "accumulator" function that checks if the current_blk's ID and
            // input port match what we're looking for
            return dst_found
                   || (dst_blk.to_string() == edge.dst_blockid
                       && dst_port == edge.dst_port);
        });
    if (dst_found) {
        // If our dst_blk is in the chain already, make sure it's the last element
        // and continue. This means we pop everything from block_chain that comes
        // after our block.
        UHD_LOG_TRACE(
            "GRAPH_UTILS", "Found dst_blk (" + dst_blk.to_string() + ") in source chain");
        while (dst_blk.to_string() != block_chain.back().dst_blockid
               || dst_port != block_chain.back().dst_port) {
            UHD_LOG_TRACE("GRAPH_UTILS",
                boost::format(
                    "Last block (%s:%d) doesn't match dst_blk (%s:%d); removing.")
                    % block_chain.back().dst_blockid % block_chain.back().dst_port
                    % dst_blk.to_string() % dst_port);
            block_chain.pop_back();
        }
    } else {
        // If we hadn't found dst_blk, find it now, then merge the two chain
        auto dest_chain = get_block_chain(graph, dst_blk, dst_port, false);
        block_chain.insert(block_chain.end(), dest_chain.begin(), dest_chain.end());
        UHD_LOG_TRACE(
            "GRAPH_UTILS", "Found destination chain for " + dst_blk.to_string());
    }

    // Finally, make all of the connections in our chain.
    // If we have SEPs in the chain, find them and directly
    // call connect on the src and dst blocks since calling
    // connect on SEPs is invalid
    std::string src_to_sep_id;
    size_t src_to_sep_port         = 0;
    bool has_src_to_sep_connection = false;
    std::string sep_to_dst_id;
    size_t sep_to_dst_port         = 0;
    bool has_sep_to_dst_connection = false;
    bool skip_pp                   = skip_property_propagation;

    for (auto edge : block_chain) {
        if (uhd::rfnoc::block_id_t(edge.dst_blockid).match(uhd::rfnoc::NODE_ID_SEP)) {
            has_src_to_sep_connection = true;
            src_to_sep_id             = edge.src_blockid;
            src_to_sep_port           = edge.src_port;
        } else if (uhd::rfnoc::block_id_t(edge.src_blockid)
                       .match(uhd::rfnoc::NODE_ID_SEP)) {
            has_sep_to_dst_connection = true;
            sep_to_dst_id             = edge.dst_blockid;
            sep_to_dst_port           = edge.dst_port;
        } else {
            graph->connect(edge.src_blockid,
                edge.src_port,
                edge.dst_blockid,
                edge.dst_port,
                skip_pp);
            skip_pp = false;
        }
    }
    if (has_src_to_sep_connection && has_sep_to_dst_connection) {
        graph->connect(
            src_to_sep_id, src_to_sep_port, sep_to_dst_id, sep_to_dst_port, skip_pp);
    } else if (has_src_to_sep_connection != has_sep_to_dst_connection) {
        const std::string err_msg = "Incomplete path. Only one SEP edge found.";
        UHD_LOG_TRACE("GRAPH_UTILS", err_msg);
        throw uhd::runtime_error("[graph_utils] " + err_msg);
    }
    return block_chain;
}

}} // namespace uhd::rfnoc
