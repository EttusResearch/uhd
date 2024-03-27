//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/factory.hpp>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace uhd::rfnoc;

/*! Pair type for device dependent block definitions. */
using block_device_pair_t = std::pair<noc_id_t, device_type_t>;


///////////////////////////////////////////////////////////////////////////////
// There are two registries:
// - The "direct" registry, which is for blocks that do not have a block
//   descriptor file
// - The "descriptor" registry, which is for blocks that *do* have a block
//   descriptor file
//
// This is the direct registry:
using block_direct_reg_t = std::unordered_map<block_device_pair_t,
    block_factory_info_t,
    boost::hash<block_device_pair_t>>;
UHD_SINGLETON_FCN(block_direct_reg_t, get_direct_block_registry);
//
// This is the descriptor registry:
using block_descriptor_reg_t =
    std::unordered_map<std::string /* block_key */, registry::factory_t>;
UHD_SINGLETON_FCN(block_descriptor_reg_t, get_descriptor_block_registry);
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * Registry functions
 *
 * Note: Don't use UHD_LOG_*, since all of this can be executed in a static
 * fashion.
 *****************************************************************************/
void registry::register_block_direct(noc_id_t noc_id,
    device_type_t device_id,
    const std::string& block_name,
    bool mb_access,
    const std::string& timebase_clock,
    const std::string& ctrlport_clock,
    factory_t factory_fn)
{
    block_device_pair_t key{noc_id, device_id};
    if (get_direct_block_registry().count(key)) {
        std::cerr << "[REGISTRY] WARNING: Attempting to overwrite previously "
                     "registered RFNoC block with noc_id,device_id: "
                  << std::hex << "0x" << noc_id << ", 0x" << device_id << std::dec
                  << std::endl;
        return;
    }
    get_direct_block_registry().emplace(key,
        block_factory_info_t{block_name,
            mb_access,
            timebase_clock,
            ctrlport_clock,
            std::move(factory_fn)});
}

void registry::register_block_direct(std::vector<noc_id_t> noc_ids,
    device_type_t device_id,
    const std::string& block_name,
    bool mb_access,
    const std::string& timebase_clock,
    const std::string& ctrlport_clock,
    factory_t factory_fn)
{
    for (auto noc_id : noc_ids) {
        register_block_direct(noc_id,
            device_id,
            block_name,
            mb_access,
            timebase_clock,
            ctrlport_clock,
            factory_fn);
    }
}

void registry::register_block_descriptor(
    const std::string& block_key, factory_t factory_fn)
{
    if (get_descriptor_block_registry().count(block_key)) {
        std::cerr << "[REGISTRY] WARNING: Attempting to overwrite previously "
                     "registered RFNoC block with block key"
                  << block_key << std::endl;
        return;
    }
    get_descriptor_block_registry().emplace(block_key, std::move(factory_fn));
}

/******************************************************************************
 * Factory functions
 *****************************************************************************/
block_factory_info_t factory::get_block_factory(noc_id_t noc_id, device_type_t device_id)
{
    // First, check the descriptor registry
    // FIXME TODO

    // Second, check the direct registry
    block_device_pair_t key{noc_id, device_id};

    if (!get_direct_block_registry().count(key)) {
        key = block_device_pair_t(noc_id, ANY_DEVICE);
    }
    if (!get_direct_block_registry().count(key)) {
        UHD_LOG_WARNING("RFNOC::BLOCK_FACTORY",
            "Could not find block with Noc-ID " << std::hex << "0x" << key.first << ", 0x"
                                                << key.second << std::dec);
        key = block_device_pair_t(DEFAULT_NOC_ID, ANY_DEVICE);
    }
    return get_direct_block_registry().at(key);
}
