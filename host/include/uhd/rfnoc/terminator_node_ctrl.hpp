//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_TERMINATOR_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_TERMINATOR_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Abstract class for terminator nodes (i.e. nodes that terminate
 *         the flow graph).
 *
 * Terminator nodes have the following properties:
 * - Data flowing into such a node is not propagated to any other node, and
 *   data coming out of this node originates in this node.
 * - Chain commands are not propagated past this node.
 *
 * A block may be a terminator node, but have both upstream and downstream
 * nodes. An example is the radio block, which can be used for Rx and Tx.
 * Even if it's used for both, the data going into the radio block is not
 * the data coming out.
 */
class UHD_RFNOC_API terminator_node_ctrl;
class terminator_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<terminator_node_ctrl> sptr;

}; /* class terminator_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_TERMINATOR_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
