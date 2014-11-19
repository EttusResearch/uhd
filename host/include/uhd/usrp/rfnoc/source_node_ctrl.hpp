//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP

#include <uhd/usrp/rfnoc/node_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Abstract class for source nodes.
 *
 * Source nodes can have downstream blocks.
 */
class UHD_API source_node_ctrl;
class source_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<source_node_ctrl> sptr;
    typedef std::map< size_t, boost::weak_ptr<source_node_ctrl> > node_map_t;
    typedef std::pair< size_t, boost::weak_ptr<source_node_ctrl> > node_map_pair_t;

    /***********************************************************************
     * Source block controls
     **********************************************************************/
    /*! Issue a stream command for this block.
     * \param stream_cmd The stream command.
     */
    virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd) = 0;

    /*! Register a node downstream of this one (i.e., a node that receives data from this node).
     *
     * Note: This does *not* affect any settings (flow control etc.). This literally only tells
     * this node about downstream nodes.
     *
     * This saves a *weak pointer* to the downstream node.
     *
     * \param downstream_node A pointer to the node instantiation
     * \param port If applicable, specify a port number the downstream block is connected to.
     */
    void register_downstream_node(
            node_ctrl_base::sptr downstream_node,
            size_t port=ANY_PORT
    );


protected:

    /*! If this function returns true, rx-specific settings (such as rx streamer
     * setup) are not propagated upstream.
     * An example for a block where this is necessary is a radio: If it's
     * operating in full duplex mode, it will have an upstream block on the tx
     * side, but we don't want to configure those while setting up an rx stream
     * chain.
     */
    bool _is_final_rx_block() { return false; };

}; /* class source_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
