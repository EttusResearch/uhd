//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_SCALAR_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_SCALAR_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Scaling node control
 *
 * A "scalar" node is a streaming node in which a scaling takes
 * place, usually for the conversion between fixed point and floating
 * point (the latter usually being normalized between -1 and 1).
 *
 * Such blocks include:
 * - Radio Controls
 * - Potentially FFTs or FIRs, if they affect scaling
 */
class UHD_RFNOC_API scalar_node_ctrl;
class scalar_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<scalar_node_ctrl> sptr;
    //! Undefined scaling
    static const double SCALE_UNDEFINED;

    /***********************************************************************
     * Scaling controls
     **********************************************************************/
    /*! Returns the scaling factor for this block on input.
     *
     * A DUC block will return the scaling factor as determined by the duc
     * stage.
     *
     * \param port Port Number
     */
    virtual double get_input_scale_factor(size_t port=ANY_PORT);

    /*! Returns the scaling factor for this block on output.
     *
     * A DDC block will return the scaling factor as determined by the ddc
     * stage.
     *
     * \param port Port Number
     */
    virtual double get_output_scale_factor(size_t port=ANY_PORT);

}; /* class scalar_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_SCALAR_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
