//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>

namespace uhd { namespace rfnoc {

class clock_iface;
class mb_controller;

/*! Data structure to hold the arguments passed into the noc_block_base ctor
 *
 * We want to hide these from the user, so she can't futz around with them.
 * Hence the opaque pointer, and non-UHD_API implementation.
 */
struct noc_block_base::make_args_t
{
    ~make_args_t();

    //! Noc-ID
    noc_id_t noc_id;

    //! Block ID (e.g. 0/Radio#0)
    block_id_t block_id;

    //! Number of input ports (gets reported from the FPGA)
    size_t num_input_ports;

    //! Number of output ports (gets reported from the FPGA)
    size_t num_output_ports;

    //! Value of the MTU register, converted to bytes
    size_t mtu;

    //! CHDR width of this block
    chdr_w_t chdr_w;

    //! Register interface to this block's register space
    register_iface::sptr reg_iface;

    //! Timebase clock interface object that is shared with the reg_iface
    std::shared_ptr<clock_iface> tb_clk_iface;

    //! Controlport clock interface object that is shared with the reg_iface
    std::shared_ptr<clock_iface> ctrlport_clk_iface;

    //! Reference to the motherboard controller associated with this block.
    //
    // Note that this may not be populated -- most blocks do not gain access to
    // the motherboard controller.
    std::shared_ptr<mb_controller> mb_control;

    //! The subtree for this block
    uhd::property_tree::sptr tree;

    //! Additional args that can be parsed and used by this block
    uhd::device_addr_t args;
};

}} /* namespace uhd::rfnoc */
