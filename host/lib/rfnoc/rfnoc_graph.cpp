//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/node.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/rfnoc/block_container.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/rfnoc_device.hpp>
#include <boost/shared_ptr.hpp> // FIXME remove when rfnoc_device is ready

using namespace uhd::rfnoc;


class rfnoc_graph_impl : public rfnoc_graph
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    rfnoc_graph_impl(const uhd::device_addr_t& dev_addr)
    {
        setup_graph(dev_addr);
    }

    ~rfnoc_graph_impl()
    {
        _graph.reset();
    }

    /**************************************************************************
     * Block Discovery/Retrieval
     *************************************************************************/
    std::vector<block_id_t> find_blocks(const std::string& block_id_hint) const
    {
        return _block_registry->find_blocks(block_id_hint);
    }

    bool has_block(const block_id_t& block_id) const
    {
        return _block_registry->has_block(block_id);
    }

    noc_block_base::sptr get_block(const block_id_t& block_id) const
    {
        return _block_registry->get_block(block_id);
    }

    /**************************************************************************
     * Graph Connections
     *************************************************************************/
    void connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        bool skip_property_propagation)
    {
        if (!has_block(src_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect blocks, source block not found: ")
                + src_blk.to_string());
        }
        if (!has_block(dst_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect blocks, source block not found: ")
                + src_blk.to_string());
        }
        _connect(get_block(src_blk),
            src_port,
            get_block(dst_blk),
            dst_port,
            skip_property_propagation);
    }

    void connect(uhd::tx_streamer& /*streamer*/,
        size_t /*strm_port*/,
        const block_id_t& /*dst_blk*/,
        size_t /*dst_port*/)
    {
        throw uhd::not_implemented_error("");
    }

    void connect(const block_id_t& /*src_blk*/,
        size_t /*src_port*/,
        uhd::rx_streamer& /*streamer*/,
        size_t /*strm_port*/)
    {
        throw uhd::not_implemented_error("");
    }

    std::shared_ptr<mb_controller> get_mb_controller(const size_t mb_index = 0)
    {
        if (!_mb_controllers.count(mb_index)) {
            throw uhd::index_error(
                std::string("Could not get mb controller for motherboard index ")
                + std::to_string(mb_index));
        }
        return _mb_controllers.at(mb_index);
    }

private:
    /**************************************************************************
     * Device Setup
     *************************************************************************/
    void setup_graph(const uhd::device_addr_t& dev_addr)
    {
        // Phase I: Initialize the motherboards
        auto dev = uhd::device::make(dev_addr);
        _device  = boost::dynamic_pointer_cast<detail::rfnoc_device>(dev);
        if (!_device) {
            throw uhd::key_error(std::string("Found no RFNoC devices for ----->\n")
                                 + dev_addr.to_pp_string());
        }

        // Configure endpoint_manager, make sure all routes are established
        // FIXME

        // Enumerate blocks, load them into the block registry
        // FIXME

        // Create graph, connect all static routes
        // FIXME
    }


    /**************************************************************************
     * Helpers
     *************************************************************************/
    /*! Internal connection helper
     *
     * Prerequisite: \p src_blk and \p dst_blk need to point to valid nodes
     */
    void _connect(std::shared_ptr<node_t> src_blk,
        size_t src_port,
        std::shared_ptr<node_t> dst_blk,
        size_t dst_port,
        bool skip_property_propagation)
    {
        graph_edge_t edge_info(
            src_port, dst_port, graph_edge_t::DYNAMIC, not skip_property_propagation);
        edge_info.src_blockid = src_blk->get_unique_id();
        edge_info.dst_blockid = dst_blk->get_unique_id();
        _graph->connect(src_blk.get(), dst_blk.get(), edge_info);
    }


    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to the underlying device implementation
    detail::rfnoc_device::sptr _device;

    //! Registry for the blocks (it's a separate class)
    std::unique_ptr<detail::block_container_t> _block_registry;

    //! Reference to the graph
    std::unique_ptr<detail::graph_t> _graph;

    //! Stash a list of motherboard controllers
    std::unordered_map<size_t, mb_controller::sptr> _mb_controllers;
}; /* class rfnoc_graph_impl */


/******************************************************************************
 * Factory
 *****************************************************************************/
rfnoc_graph::sptr rfnoc_graph::make(const uhd::device_addr_t& device_addr)
{
    return std::make_shared<rfnoc_graph_impl>(device_addr);
}
