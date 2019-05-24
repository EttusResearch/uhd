//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/exception.hpp>

using namespace uhd::rfnoc;

/******************************************************************************
 * Structors
 *****************************************************************************/
noc_block_base::noc_block_base(make_args_ptr make_args)
    : register_iface_holder(std::move(make_args->reg_iface))
    , _noc_id(make_args->noc_id)
    , _block_id(make_args->block_id)
    , _num_input_ports(make_args->num_input_ports)
    , _num_output_ports(make_args->num_output_ports)
{
}

noc_block_base::~noc_block_base()
{
    // nop
}

