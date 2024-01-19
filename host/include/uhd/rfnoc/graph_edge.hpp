//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <string>
#include <tuple>

namespace uhd { namespace rfnoc {

/*! A container that holds information about a graph edge
 *
 * Note: The source and destination IDs are strings, not block IDs
 * (uhd::rfnoc::block_id_t). This is because the graph can contain edges
 * that are not between RFNoC blocks (e.g., to a streamer), and we need to
 * be able to generically express node IDs.
 */
struct UHD_API graph_edge_t
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
        const bool fwd_edge)
        : src_port(src_port_), dst_port(dst_port_), edge(edge_), is_forward_edge(fwd_edge)
    {
    }

    //! The ID of the source block for this edge
    std::string src_blockid;
    //! The port number of the source block for this edge
    size_t src_port = 0;
    //! The ID of the destination block for this edge
    std::string dst_blockid;
    //! The port number of the destination block for this edge
    size_t dst_port = 0;
    //! The type of edge
    edge_t edge = DYNAMIC;
    //! When false, the framework will assume this is a back-edge. Back-edges
    // are not used for sorting the graph as a DAG.
    bool is_forward_edge = true;

    //! Equality operator: Compare two edges if they match, including edge
    // properties.
    bool operator==(const graph_edge_t& rhs) const
    {
        return is_equal(rhs, true);
    }

    /*! Equality comparison of two edges.
     *
     * If \p match_properties is false, this compares two edges to test if they
     * have the same direction, source port, and destination port.
     * If \p match_properties is true, then it also tests if all the edge
     * properties match (edge type, back-edge).
     *
     * \returns true if edges match.
     */
    bool is_equal(const graph_edge_t& rhs, const bool match_properties = false) const
    {
        return (std::tie(src_blockid, src_port, dst_blockid, dst_port)
                   == std::tie(
                       rhs.src_blockid, rhs.src_port, rhs.dst_blockid, rhs.dst_port))
               && (match_properties ? (std::tie(edge, is_forward_edge)
                                       == std::tie(rhs.edge, rhs.is_forward_edge))
                                    : true);
    }

    //! Return a string representation of the connection
    std::string to_string() const
    {
        return src_blockid + ":" + std::to_string(src_port)
               + (edge == STATIC ? "==>" : "-->") + dst_blockid + ":"
               + std::to_string(dst_port);
    }
};


}} /* namespace uhd::rfnoc */
