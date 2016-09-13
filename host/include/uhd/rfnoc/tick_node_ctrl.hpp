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

#ifndef INCLUDED_LIBUHD_TICK_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_TICK_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Tick-rate-aware node control
 *
 * A "rate" node is a streaming node is a point in the flow graph
 * that is aware of tick rates (time base). Such nodes include:
 * - Radio Controls
 * - Data generating blocks that add time stamps
 */
class UHD_RFNOC_API tick_node_ctrl;
class tick_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<tick_node_ctrl> sptr;

    /***********************************************************************
     * Constants
     **********************************************************************/
    //! This value is used by rate nodes that don't actually set a rate themselves
    static const double RATE_UNDEFINED;

    /***********************************************************************
     * Rate controls
     **********************************************************************/
    /*! Return a tick rate.
     *
     * This might be either a tick rate defined by this block (see also _get_tick_rate())
     * or it's a tick rate defined by an adjacent block.
     * In that case, performs a graph search to figure out the tick rate.
     */
    double get_tick_rate(
            const std::set< node_ctrl_base::sptr > &_explored_nodes=std::set< node_ctrl_base::sptr >()
    );

protected:
    virtual double _get_tick_rate() { return RATE_UNDEFINED; };

}; /* class tick_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_TICK_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
