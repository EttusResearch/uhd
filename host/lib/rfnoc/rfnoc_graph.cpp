//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/noc_block_make_args.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhdlib/rfnoc/block_container.hpp>
#include <uhdlib/rfnoc/factory.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/graph_stream_manager.hpp>
#include <uhdlib/rfnoc/rfnoc_device.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/shared_ptr.hpp> // FIXME remove when rfnoc_device is ready
#include <memory>

using namespace uhd::rfnoc;


class rfnoc_graph_impl : public rfnoc_graph
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    rfnoc_graph_impl(const uhd::device_addr_t& dev_addr)
        : _block_registry(std::make_unique<detail::block_container_t>())
        , _graph(std::make_unique<uhd::rfnoc::detail::graph_t>())
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
        _physical_connect(src_blk, src_port, dst_blk, dst_port);
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


    size_t get_num_mboards() const
    {
        return _num_mboards;
    }

    void commit()
    {
        _graph->commit();
    }

    void release()
    {
        _graph->release();
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

        // Create a graph stream manager
        // FIXME get these from mb_iface or something
        static const chdr::chdr_packet_factory pkt_factory(
            CHDR_W_64, uhd::ENDIANNESS_BIG);
        epid_allocator::sptr epid_alloc = std::make_shared<epid_allocator>();
        // Create a collection of link definitions: (ID, MB) pairs
        std::vector<std::pair<device_id_t, mb_iface*>> links;
        // TODO fix device_id we're creating
        links.push_back(std::make_pair(100, &_device->get_mb_iface(0)));
        try {
            _gsm = graph_stream_manager::make(pkt_factory, epid_alloc, links);
        } catch (uhd::io_error& ex) {
            UHD_LOG_ERROR("RFNOC::GRAPH", "IO Error during GSM initialization. " << ex.what());
            throw;
        }

        // Configure endpoint_manager, make sure all routes are established
        // FIXME

        // Enumerate blocks, load them into the block registry
        // Iterate through the mboards
        for (size_t mb_idx = 0; mb_idx < get_num_mboards(); ++mb_idx) {
            // Setup the interfaces for this mboard and get some configuration info
            mb_iface& mb = _device->get_mb_iface(mb_idx);
            // Ask GSM to allow us to talk to our remote mb
            sep_addr_t ctrl_sep_addr(mb.get_remote_device_id(), 0);
            _gsm->connect_host_to_device(ctrl_sep_addr);
            detail::client_zero::sptr mb_cz = _gsm->get_client_zero(ctrl_sep_addr);
            const size_t num_blocks         = mb_cz->get_num_blocks();
            const size_t first_block_port   = 1 + mb_cz->get_num_stream_endpoints();

            // Make a map to count the number of each block we have
            std::unordered_map<std::string, uint16_t> block_count_map;

            // Iterate through and register each of the blocks in this mboard
            for (size_t portno = 0; portno < num_blocks; ++portno) {
                auto noc_id = mb_cz->get_noc_id(portno + first_block_port);
                auto block_factory_pair = factory::get_block_factory(noc_id);
                auto block_info = mb_cz->get_block_info(portno + first_block_port);
                block_id_t block_id(mb_idx,
                    block_factory_pair.second,
                    block_count_map[block_factory_pair.second]++);
                auto clk_iface = std::make_shared<clock_iface>(block_id.to_string() + "_clock");
                auto block_reg_iface = _gsm->get_block_register_iface(ctrl_sep_addr,
                    portno,
                    *clk_iface.get(),
                    *clk_iface.get());
                auto make_args_uptr = std::make_unique<noc_block_base::make_args_t>();
                make_args_uptr->noc_id = noc_id;
                make_args_uptr->block_id = block_id;
                make_args_uptr->num_input_ports = block_info.num_inputs;
                make_args_uptr->num_output_ports = block_info.num_outputs;
                make_args_uptr->reg_iface = block_reg_iface;
                make_args_uptr->clk_iface = clk_iface;
                make_args_uptr->mb_control = (factory::has_requested_mb_access(noc_id) ? _mb_controllers.at(mb_idx) : nullptr);
                make_args_uptr->tree = _tree->subtree("/mboards/0"); /* FIXME Get the block's subtree */
                make_args_uptr->args = dev_addr; // TODO filter the device args
                _block_registry->register_block(block_factory_pair.first(std::move(make_args_uptr)));
                _xbar_block_config[block_id.to_string()] = {
                    portno, noc_id, block_id.get_block_count()};
            }
        }

        // Create graph, connect all static routes
        // FIXME
    }

    /**************************************************************************
     * Helpers
     *************************************************************************/
    /*! Internal connection helper
     *
     * Make the connections in the _graph, and set up property propagation
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

    /*! Internal physical connection helper
     *
     * Make the connections in the physical device
     *
     * \throws connect_disallowed_on_src
     *     if the source port is statically connected to a *different* block
     * \throws connect_disallowed_on_dst
     *     if the destination port is statically connected to a *different* block
     */
    void _physical_connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port)
    {
        auto src_blk_ctrl = get_block(src_blk);
        auto dst_blk_ctrl = get_block(dst_blk);

        /*
         * Start by determining if the connection can be made
         * Get the adjacency list and check if the connection is in it already
         */
        // Read the adjacency list for the source and destination blocks
        auto src_mb_idx = src_blk.get_device_no();
        auto src_cz     = _gsm->get_client_zero(
            sep_addr_t(_device->get_mb_iface(src_mb_idx).get_remote_device_id(), 0));
        std::vector<detail::client_zero::edge_def_t>& adj_list =
            src_cz->get_adjacency_list();
        // Check the src_blk
        auto src_blk_xbar_info =
            _xbar_block_config.at(src_blk_ctrl->get_block_id().to_string());
        // This "xbar_port" starts at the first block, so we need to add the client zero
        // and stream endpoint ports
        const auto src_xbar_port =
            src_blk_xbar_info.xbar_port + src_cz->get_num_stream_endpoints() + 1;
        // We can also find out which stream endpoint the src block is connected to
        sep_inst_t src_sep;
        for (detail::client_zero::edge_def_t edge : adj_list) {
            if ((edge.src_blk_index == src_xbar_port)
                and (edge.src_blk_port == src_port)) {
                UHD_LOGGER_DEBUG("RFNOC::GRAPH")
                    << boost::format("Block found in adjacency list. %d:%d->%d:%d")
                           % edge.src_blk_index
                           % static_cast<unsigned int>(edge.src_blk_port)
                           % edge.dst_blk_index
                           % static_cast<unsigned int>(edge.dst_blk_port);
                if (edge.dst_blk_index <= src_cz->get_num_stream_endpoints()) {
                    src_sep =
                        edge.dst_blk_index - 1 /* minus 1 because port 0 is client zero*/;
                } else {
                    // TODO connect_disallowed_on_src?
                    // TODO put more info in exception
                    throw uhd::routing_error(
                        "Unable to connect to statically connected source port");
                }
            }
        }

        // Read the dst adjacency list if its different
        // TODO they may be on the same mboard, which would make this redundant
        auto dst_mb_idx = dst_blk.get_device_no();
        auto dst_cz     = _gsm->get_client_zero(
            sep_addr_t(_device->get_mb_iface(dst_mb_idx).get_remote_device_id(), 0));
        adj_list = dst_cz->get_adjacency_list();
        // Check the dst blk
        auto dst_blk_xbar_info =
            _xbar_block_config.at(dst_blk_ctrl->get_block_id().to_string());
        // This "xbar_port" starts at the first block, so we need to add the client zero
        // and stream endpoint ports
        const auto dst_xbar_port =
            dst_blk_xbar_info.xbar_port + dst_cz->get_num_stream_endpoints() + 1;
        // We can also find out which stream endpoint the dst block is connected to
        sep_inst_t dst_sep;
        for (detail::client_zero::edge_def_t edge : adj_list) {
            if ((edge.dst_blk_index == dst_xbar_port)
                and (edge.dst_blk_port == dst_port)) {
                UHD_LOGGER_DEBUG("RFNOC::GRAPH")
                    << boost::format("Block found in adjacency list. %d:%d->%d:%d")
                           % edge.src_blk_index
                           % static_cast<unsigned int>(edge.src_blk_port)
                           % edge.dst_blk_index
                           % static_cast<unsigned int>(edge.dst_blk_port);
                if (edge.src_blk_index <= dst_cz->get_num_stream_endpoints()) {
                    dst_sep =
                        edge.src_blk_index - 1 /* minus 1 because port 0 is client zero*/;
                } else {
                    // TODO connect_disallowed_on_dst?
                    // TODO put more info in exception
                    throw uhd::routing_error(
                        "Unable to connect to statically connected destination port");
                }
            }
        }

        /* TODO: we checked if either port is used in a static connection (and its not if
         * we've made it this far). We also need to check something else, but I can't
         * remember what...
         */

        // At this point, we know the attempted connection is valid, so let's go ahead and
        // make it
        sep_addr_t src_sep_addr(
            _device->get_mb_iface(src_mb_idx).get_remote_device_id(), src_sep);
        sep_addr_t dst_sep_addr(
            _device->get_mb_iface(dst_mb_idx).get_remote_device_id(), dst_sep);
        auto strm_info = _gsm->create_device_to_device_data_stream(
            dst_sep_addr, src_sep_addr, false, 0.1, 0.0, false);

        UHD_LOGGER_INFO("RFNOC::GRAPH")
            << boost::format("Data stream between EPID %d and EPID %d established "
                             "where downstream buffer can hold %lu bytes and %u packets")
                   % std::get<0>(strm_info).first % std::get<0>(strm_info).second
                   % std::get<1>(strm_info).bytes % std::get<1>(strm_info).packets;
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to the underlying device implementation
    detail::rfnoc_device::sptr _device;

    //! Number of motherboards, this is technically redundant but useful for
    // easy lookups.
    size_t _num_mboards;

    //! Reference to the property tree
    uhd::property_tree::sptr _tree;

    //! Registry for the blocks (it's a separate class)
    std::unique_ptr<detail::block_container_t> _block_registry;

    /*! Registry for the actual block connections on the crossbar
     *  When we register blocks in the _block_registry, we also need to store some
     * information in this map so we can easily figure out which crossbar port a block
     * controller is connected to
     * \p keys are the string representation of the block ID
     * \p values are the block crossbar information structs
     * TODO: change from string block IDs to block_id_t with COOL custom hashing
     */
    std::unordered_map<std::string, block_xbar_info> _xbar_block_config;

    //! Reference to the graph
    std::unique_ptr<detail::graph_t> _graph;

    //! Stash a list of motherboard controllers
    std::unordered_map<size_t, mb_controller::sptr> _mb_controllers;

    //! Stash of the client zeros for all motherboards
    std::unordered_map<size_t, detail::client_zero::sptr> _client_zeros;

    //! uptr to graph stream manager
    graph_stream_manager::uptr _gsm;
}; /* class rfnoc_graph_impl */


/******************************************************************************
 * Factory
 *****************************************************************************/
rfnoc_graph::sptr rfnoc_graph::make(const uhd::device_addr_t& device_addr)
{
    return std::make_shared<rfnoc_graph_impl>(device_addr);
}
