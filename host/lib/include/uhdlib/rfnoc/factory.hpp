//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/registry.hpp>

namespace uhd { namespace rfnoc {

struct block_factory_info_t
{
    std::string block_name;
    bool mb_access;
    std::string timebase_clk;
    std::string ctrlport_clk;
    registry::factory_t factory_fn;
};

/*! Container for factory functionality
 */
class factory
{
public:
    /*! Return a factory function for an RFNoC block based on the Noc-ID
     *
     * \returns a block_factory_info_t object
     * \throws uhd::lookup_error if no block is found
     */
    static block_factory_info_t get_block_factory(
        noc_id_t noc_id, device_type_t device_id);
};

}} /* namespace uhd::rfnoc */
