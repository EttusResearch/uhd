//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_NOC_BLOCK_MAKE_ARGS_HPP
#define INCLUDED_LIBUHD_NOC_BLOCK_MAKE_ARGS_HPP

#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/property_tree.hpp>

namespace uhd { namespace rfnoc {

/*! Data structure to hold the arguments passed into the noc_block_base ctor
 *
 * We want to hide these from the user, so she can't futz around with them.
 * Hence the opaque pointer, and non-UHD_API implementation.
 */
struct noc_block_base::make_args_t
{
    //! Noc-ID
    noc_id_t noc_id;

    //! Block ID (e.g. 0/Radio#0)
    block_id_t block_id;

    //! Number of input ports (gets reported from the FPGA)
    size_t num_input_ports;

    //! Number of output ports (gets reported from the FPGA)
    size_t num_output_ports;

    //! Register interface to this block's register space
    register_iface::sptr reg_iface;

    //! The subtree for this block
    uhd::property_tree::sptr tree;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_NOC_BLOCK_MAKE_ARGS_HPP */
