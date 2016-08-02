//
// Copyright 2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
