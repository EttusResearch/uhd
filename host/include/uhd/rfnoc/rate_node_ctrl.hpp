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

#ifndef INCLUDED_LIBUHD_RATE_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_RATE_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Sampling-rate-aware node control
 *
 * A "rate" node is a streaming node is a point in the flow graph
 * that is aware of sampling rates. Such nodes include:
 * - Radio Controls (these actually set a sampling rate)
 * - Decimating FIR filters (their output rates depend on both
 *   incoming rates and their own settings, i.e. the decimation)
 * - Streaming terminators (these need to know the sampling rates
 *   to configure the connected streamers)
 */
class UHD_RFNOC_API rate_node_ctrl;
class rate_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<rate_node_ctrl> sptr;
    //! This value is used by rate nodes that don't actually set a rate themselves
    static const double RATE_UNDEFINED;

    /***********************************************************************
     * Rate controls
     **********************************************************************/
    /*! Returns the sampling rate this block expects at its input.
     *
     * A radio will simply return the sampling rate it is set to.
     * A decimating FIR filter will ask downstream for the input sampling rate
     * and then return that value multiplied by the decimation factor.
     *
     */
    virtual double get_input_samp_rate(size_t port=ANY_PORT);
    virtual double get_output_samp_rate(size_t port=ANY_PORT);

}; /* class rate_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RATE_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:

