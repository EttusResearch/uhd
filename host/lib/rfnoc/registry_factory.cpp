//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/factory.hpp>
#include <unordered_map>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace uhd::rfnoc;

///////////////////////////////////////////////////////////////////////////////
// There are two registries:
// - The "direct" registry, which is for blocks that do not have a block
//   descriptor file
// - The "descriptor" registry, which is for blocks that *do* have a block
//   descriptor file
//
// This is the direct registry:
using block_direct_reg_t = std::unordered_map<noc_block_base::noc_id_t,
    std::tuple<std::string /* block_name */,
        registry::factory_t>>;
UHD_SINGLETON_FCN(block_direct_reg_t, get_direct_block_registry);
//
// This is the descriptor registry:
using block_descriptor_reg_t =
    std::unordered_map<std::string /* block_key */, registry::factory_t>;
UHD_SINGLETON_FCN(block_descriptor_reg_t, get_descriptor_block_registry);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// These registries are for blocks that have requested motherboard access
using block_direct_mb_access_req_t = std::unordered_set<noc_block_base::noc_id_t>;
UHD_SINGLETON_FCN(block_direct_mb_access_req_t, get_direct_block_mb_access_requested);
//
// This is the descriptor registry:
using block_descriptor_mb_access_req_t = std::unordered_set<std::string>;
UHD_SINGLETON_FCN(
    block_descriptor_mb_access_req_t, get_descriptor_block_mb_access_requested);
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * Registry functions
 *
 * Note: Don't use UHD_LOG_*, since all of this can be executed in a static
 * fashion.
 *****************************************************************************/
void registry::register_block_direct(noc_block_base::noc_id_t noc_id,
    const std::string& block_name,
    factory_t factory_fn)
{
    if (get_direct_block_registry().count(noc_id)) {
        std::cerr
            << "[REGISTRY] WARNING: Attempting to overwrite previously registered RFNoC "
               "block with Noc-ID 0x"
            << std::hex << noc_id << std::dec << std::endl;
        return;
    }
    get_direct_block_registry().emplace(
        noc_id, std::make_tuple(block_name, std::move(factory_fn)));
}

void registry::register_block_descriptor(
    const std::string& block_key, factory_t factory_fn)
{
    if (get_descriptor_block_registry().count(block_key)) {
        std::cerr << "WARNING: Attempting to overwriting previously registered RFNoC "
                     "block with block key"
                  << block_key << std::endl;
        return;
    }
    get_descriptor_block_registry().emplace(block_key, std::move(factory_fn));
}

void registry::request_mb_access(noc_block_base::noc_id_t noc_id)
{
    if (!get_direct_block_mb_access_requested().count(noc_id)) {
        get_direct_block_mb_access_requested().emplace(noc_id);
    }
}

void registry::request_mb_access(const std::string& block_key)
{
    if (!get_descriptor_block_mb_access_requested().count(block_key)) {
        get_descriptor_block_mb_access_requested().emplace(block_key);
    }
}

/******************************************************************************
 * Factory functions
 *****************************************************************************/
std::pair<registry::factory_t, std::string> factory::get_block_factory(
    noc_block_base::noc_id_t noc_id)
{
    // First, check the descriptor registry
    // FIXME TODO

    // Second, check the direct registry
    if (!get_direct_block_registry().count(noc_id)) {
        UHD_LOG_WARNING("RFNOC::BLOCK_FACTORY",
            "Could not find block with Noc-ID "
                << std::hex << std::setw(sizeof(noc_block_base::noc_id_t) * 2) << noc_id);
        noc_id = DEFAULT_NOC_ID;
    }
    auto& block_info = get_direct_block_registry().at(noc_id);
    return {std::get<1>(block_info), std::get<0>(block_info)};
}

bool factory::has_requested_mb_access(noc_block_base::noc_id_t noc_id)
{
    if (get_direct_block_mb_access_requested().count(noc_id)) {
        return true;
    }

    // FIXME tbw:
    // - Map noc_id to block key
    // - Check that key's descriptor
    // - If that block has requested MB access, stash the noc ID in
    // get_direct_block_mb_access_requested() for faster lookups in the future

    return false;
}
