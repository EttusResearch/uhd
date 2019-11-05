//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/block_container.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/format.hpp>
#include <algorithm>


using namespace uhd::rfnoc::detail;

using uhd::rfnoc::block_id_t;
using uhd::rfnoc::noc_block_base;

/******************************************************************************
 * Structors
 *****************************************************************************/
block_container_t::block_container_t()
{
    // nop
}


/******************************************************************************
 * API
 *****************************************************************************/
void block_container_t::register_block(noc_block_base::sptr block)
{
    std::lock_guard<std::mutex> lock(_mutex);
    UHD_LOGGER_DEBUG("RFNOC::BLOCK_CONTAINER")
        << boost::format("Registering block: %s (NOC ID=%08x)") % block->get_unique_id()
               % block->get_noc_id();
    _blocks.insert(block);
}

std::vector<block_id_t> block_container_t::find_blocks(
    const std::string& block_id_hint) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<block_id_t> block_ids;
    for (auto it = _blocks.cbegin(); it != _blocks.cend(); ++it) {
        auto id = (*it)->get_block_id();
        if (id.match(block_id_hint) || block_id_hint.empty()) {
            block_ids.push_back(id);
        }
    }
    std::sort(block_ids.begin(),
        block_ids.end(),
        [](const uhd::rfnoc::block_id_t& i, const uhd::rfnoc::block_id_t& j) {
            return i < j;
        });
    return block_ids;
}

bool block_container_t::has_block(const block_id_t& block_id) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::any_of(
        _blocks.cbegin(), _blocks.cend(), [block_id](noc_block_base::sptr block) {
            return block->get_block_id() == block_id;
        });
}

noc_block_base::sptr block_container_t::get_block(const block_id_t& block_id) const
{
    auto block_itr = std::find_if(
        _blocks.cbegin(), _blocks.cend(), [block_id](noc_block_base::sptr block) {
            return block->get_block_id() == block_id;
        });
    if (block_itr == _blocks.cend()) {
        throw uhd::lookup_error(std::string("This device does not have a block with ID: ")
                                + block_id.to_string());
    }
    return *block_itr;
}

void block_container_t::shutdown()
{
    node_accessor_t node_accessor{};
    for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
        node_accessor.shutdown(it->get());
    }
}

void block_container_t::init_props()
{
    node_accessor_t node_accessor{};
    for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
        node_accessor.init_props(it->get());
    }
}
