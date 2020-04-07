//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <unordered_set>
#include <boost/units/detail/utility.hpp>
#include <mutex>
#include <vector>

namespace uhd { namespace rfnoc { namespace detail {

/*! Storage container for RFNoC block controllers
 */
class block_container_t
{
public:
    block_container_t();

    void register_block(noc_block_base::sptr block);

    /*! Returns the block ids of all blocks that match the specified hint
     *
     * See uhd::rfnoc::rfnoc_graph::find_blocks() for details.
     */
    std::vector<block_id_t> find_blocks(const std::string& block_id_hint) const;

    /*! Checks if a specific NoC block exists on the device.
     *
     * See uhd::rfnoc::rfnoc_graph::has_block() for details.
     */
    bool has_block(const block_id_t& block_id) const;

    /*! \brief Returns a block controller class for an NoC block.
     *
     * See uhd::rfnoc::rfnoc_graph::get_block() for details.
     */
    noc_block_base::sptr get_block(const block_id_t& block_id) const;

    /*! Initialize properties on all registered blocks
     */
    void init_props();

    /*! Call shutdown() on all blocks
     *
     * After calling this, blocks won't be able to do anything anymore!
     */
    void shutdown();

private:
    //! Lock access to the storage
    mutable std::mutex _mutex;

    //! The actual block registry
    std::unordered_set<noc_block_base::sptr> _blocks;
};

}}} /* namespace uhd::rfnoc::detail */
