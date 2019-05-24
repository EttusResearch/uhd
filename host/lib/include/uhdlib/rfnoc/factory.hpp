//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_FACTORY_HPP
#define INCLUDED_LIBUHD_RFNOC_FACTORY_HPP

#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Container for factory functionality
 */
class factory
{
public:
    /*! Return a factory function for an RFNoC block based on the Noc-ID
     *
     * \returns a pair: factory function, and block name
     * \throws uhd::lookup_error if no block is found
     */
    static std::pair<registry::factory_t, std::string>
    get_block_factory(noc_block_base::noc_id_t noc_id);
};


}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_FACTORY_HPP */
