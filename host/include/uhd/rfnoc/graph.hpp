//
// Copyright 2016 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_RFNOC_GRAPH_HPP
#define INCLUDED_LIBUHD_RFNOC_GRAPH_HPP

#include <boost/noncopyable.hpp>
#include <uhd/rfnoc/block_id.hpp>

namespace uhd { namespace rfnoc {

class graph : boost::noncopyable
{
public:
    typedef boost::shared_ptr<uhd::rfnoc::graph> sptr;

    /*! Connect a RFNOC block with block ID \p src_block to another with block ID \p dst_block.
     *
     * This will:
     * - Check if this connection is valid (IO signatures, see if types match)
     * - Configure the flow control for the blocks
     * - Configure SID for the upstream block
     * - Register the upstream block in the downstream block
     */
    virtual void connect(
                const block_id_t &src_block,
                size_t src_block_port,
                const block_id_t &dst_block,
                size_t dst_block_port,
                const size_t pkt_size = 0
    ) = 0;

    /*! Shorthand for connect().
     *
     * Using default ports for both source and destination.
     */
    virtual void connect(
            const block_id_t &src_block,
            const block_id_t &dst_block
    ) = 0;

    virtual std::string get_name() const = 0;
};

}}; /* name space uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_GRAPH_HPP */
// vim: sw=4 et:
