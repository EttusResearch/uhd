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

#ifndef INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP

#include <uhd/usrp/rfnoc/node_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/usrp/rfnoc/sink_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief TODO write
 */
class UHD_API sink_node_ctrl;
class sink_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<sink_node_ctrl> sptr;
    typedef std::map< size_t, boost::weak_ptr<sink_node_ctrl> > node_map_t;
    typedef std::pair< size_t, boost::weak_ptr<sink_node_ctrl> > node_map_pair_t;

    /***********************************************************************
     * Sink block controls
     **********************************************************************/
    /*! Register a node upstream of this one (i.e., a node that can send data to this node).
     *
     * Note: This does *not* affect any settings (flow control etc.). This literally only tells
     * this node about upstream nodes.
     *
     * By definition, the upstream node must of type source_node_ctrl.
     *
     * This saves a *weak pointer* to the upstream node.
     *
     * \param upstream_node A pointer to the node instantiation
     * \param port If applicable, specify a port number the upstream block is connected to.
     */
    void register_upstream_node(
            node_ctrl_base::sptr upstream_node,
            size_t port=ANY_PORT
    );

protected:

    /*! If this function returns true, tx-specific settings (such as tx streamer
     * setup) are not propagated downstream.
     * An example for a block where this is necessary is a radio: If it's
     * operating in full duplex mode, it will have a downstream block on the rx
     * side, but we don't want to configure those while setting up an tx stream
     * chain.
     */
    bool _is_final_tx_block() { return false; };

}; /* class sink_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
