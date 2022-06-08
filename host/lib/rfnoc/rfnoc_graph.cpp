//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/constants.hpp>
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
#include <uhdlib/rfnoc/rfnoc_rx_streamer.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer.hpp>
#include <uhdlib/usrp/common/io_service_mgr.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <memory>

using namespace uhd;
using namespace uhd::rfnoc;

namespace {
const std::string LOG_ID("RFNOC::GRAPH");

//! Which blocks are actually stored at a given port on the crossbar
struct block_xbar_info
{
    size_t xbar_port;
    noc_id_t noc_id;
    size_t inst_num;
};

//! Information about a specific connection (used for streamer disconnect)
struct connection_info_t
{
    detail::graph_t::node_ref_t src;
    detail::graph_t::node_ref_t dst;
    graph_edge_t edge;
};

//! Information about a streamer (used for streamer disconnect)
struct streamer_info_t
{
    detail::graph_t::node_ref_t node;
    std::map<size_t, connection_info_t> connections;
};

//! Information about a route (used for physical connect/disconnect)
struct route_info_t
{
    graph_edge_t::edge_t edge_type;
    graph_edge_t src_static_edge;
    graph_edge_t dst_static_edge;
};

} // namespace

// Define an attorney to limit access to noc_block_base internals
class rfnoc_graph_impl;
namespace uhd { namespace rfnoc {

class block_initializer
{
    static void post_init(noc_block_base::sptr block)
    {
        block->post_init();
    }
    friend rfnoc_graph_impl;
};

}} // namespace uhd::rfnoc


class rfnoc_graph_impl : public rfnoc_graph
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    rfnoc_graph_impl(
        detail::rfnoc_device::sptr dev, const uhd::device_addr_t& dev_addr) try
        : _device(dev),
          _tree(_device->get_tree()),
          _num_mboards(_tree->list("/mboards").size()),
          _block_registry(std::make_unique<detail::block_container_t>()),
          _graph(std::make_unique<uhd::rfnoc::detail::graph_t>()) {
        _mb_controllers.reserve(_num_mboards);
        // Now initialize all subsystems:
        _init_io_srv_mgr(dev_addr); // Global I/O Service Manager
        _init_mb_controllers();
        _init_gsm(); // Graph Stream Manager
        try {
            // If anything fails here, we immediately deinit all the other
            // blocks to avoid any more fallout, then safely bring down the
            // device.
            for (size_t mb_idx = 0; mb_idx < _num_mboards; ++mb_idx) {
                _init_blocks(mb_idx, dev_addr);
            }
            UHD_LOG_TRACE(LOG_ID, "Initializing properties on all blocks...");
            _block_registry->init_props();
            _init_sep_map();
            _init_static_connections();
            _init_mbc();
            // Start with time set to zero, but don't complain if sync fails
            rfnoc_graph_impl::synchronize_devices(uhd::time_spec_t(0.0), true);
        } catch (...) {
            _block_registry->shutdown();
            throw;
        }
    } catch (const std::exception& ex) {
        UHD_LOG_ERROR(LOG_ID, "Caught exception while initializing graph: " << ex.what());
        throw uhd::runtime_error("Failure to create rfnoc_graph.");
    } catch (...) {
        UHD_LOG_ERROR(LOG_ID, "Caught unknown exception while initializing graph!");
        throw uhd::runtime_error("Failure to create rfnoc_graph.");
    }

    ~rfnoc_graph_impl() override
    {
        UHD_LOG_TRACE(LOG_ID, "Shutting down detail::graph...");
        _graph->shutdown();
        UHD_LOG_TRACE(LOG_ID, "Shutting down all blocks ...");
        _block_registry->shutdown();
        _graph.reset();
    }

    /**************************************************************************
     * Block Discovery/Retrieval
     *************************************************************************/
    std::vector<block_id_t> find_blocks(const std::string& block_id_hint) const override
    {
        return _block_registry->find_blocks(block_id_hint);
    }

    bool has_block(const block_id_t& block_id) const override
    {
        return _block_registry->has_block(block_id);
    }

    noc_block_base::sptr get_block(const block_id_t& block_id) const override
    {
        return _block_registry->get_block(block_id);
    }

    /**************************************************************************
     * Graph Connections
     *************************************************************************/
    bool is_connectable(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port) override
    {
        try {
            const std::string src_blk_info =
                src_blk.to_string() + ":" + std::to_string(src_port);
            const std::string dst_blk_info =
                dst_blk.to_string() + ":" + std::to_string(dst_port);

            // Find the static edge for src_blk:src_port
            auto src_static_edge_o = _get_static_edge(
                [src_blk_id = src_blk.to_string(), src_port](const graph_edge_t& edge) {
                    return edge.src_blockid == src_blk_id && edge.src_port == src_port;
                });
            // If the edge doesn't exist, then it's not even connected in the
            // FPGA.
            if (!src_static_edge_o) {
                return false;
            }
            graph_edge_t src_static_edge = src_static_edge_o.get();

            // Now see if it's already connected to the destination
            if (src_static_edge.dst_blockid == dst_blk.to_string()
                && src_static_edge.dst_port == dst_port) {
                return true;
            }

            // If they're not statically connected, the source *must* be connected
            // to an SEP, or this route is impossible
            if (block_id_t(src_static_edge.dst_blockid).get_block_name() != NODE_ID_SEP) {
                return false;
            }

            // OK, now we know which source SEP we have
            const std::string src_sep_info = src_static_edge.dst_blockid;
            // const sep_addr_t src_sep_addr  = _sep_map.at(src_sep_info);

            // Now find the static edge for the destination SEP
            auto dst_static_edge_o = _get_static_edge(
                [dst_blk_id = dst_blk.to_string(), dst_port](const graph_edge_t& edge) {
                    return edge.dst_blockid == dst_blk_id && edge.dst_port == dst_port;
                });
            // If the edge doesn't exist, then it's not even connected in the
            // FPGA.
            if (!dst_static_edge_o) {
                return false;
            }
            graph_edge_t dst_static_edge = dst_static_edge_o.get();

            // If they're not statically connected, the source *must* be connected
            // to an SEP, or this route is impossible
            if (block_id_t(dst_static_edge.src_blockid).get_block_name() != NODE_ID_SEP) {
                return false;
            }

            // OK, now we know which destination SEP we have
            const std::string dst_sep_info = dst_static_edge.src_blockid;
            // const sep_addr_t dst_sep_addr  = _sep_map.at(dst_sep_info);

            UHD_LOG_WARNING(LOG_ID,
                "is_connectable() currently assuming that SEPs"
                    << dst_sep_info << " and " << src_sep_info
                    << " are connectable. "
                       "Please implement a better check.");
        } catch (...) {
            return false;
        }
        return true;
    }


    void connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        bool is_back_edge) override
    {
        if (!has_block(src_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect blocks, source block not found: ")
                + src_blk.to_string());
        }
        if (!has_block(dst_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect blocks, destination block not found: ")
                + dst_blk.to_string());
        }
        auto edge_type = _physical_connect(src_blk, src_port, dst_blk, dst_port);
        _connect(get_block(src_blk),
            src_port,
            get_block(dst_blk),
            dst_port,
            edge_type,
            is_back_edge);
    }

    void disconnect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port) override
    {
        if (not has_block(src_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot disconnect blocks, source block not found: ")
                + src_blk.to_string());
        }
        if (not has_block(dst_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot disconnect blocks, destination block not found: ")
                + dst_blk.to_string());
        }
        auto edge_type = _physical_disconnect(src_blk, src_port, dst_blk, dst_port);
        graph_edge_t edge_info(src_port, dst_port, edge_type, true);
        auto src              = get_block(src_blk);
        auto dst              = get_block(dst_blk);
        edge_info.src_blockid = src->get_unique_id();
        edge_info.dst_blockid = dst->get_unique_id();
        _graph->disconnect(src.get(), dst.get(), edge_info);
    }

    void connect(uhd::tx_streamer::sptr streamer,
        size_t strm_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        uhd::transport::adapter_id_t adapter_id) override
    {
        // Verify the streamer was created by us
        auto rfnoc_streamer = std::dynamic_pointer_cast<rfnoc_tx_streamer>(streamer);
        if (!rfnoc_streamer) {
            throw uhd::type_error("Streamer is not rfnoc capable");
        }

        // Verify src_blk even exists in this graph
        if (!has_block(dst_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect block to streamer, source block not found: ")
                + dst_blk.to_string());
        }

        // Verify src_blk has an SEP upstream
        graph_edge_t dst_static_edge = _assert_edge(
            _get_static_edge(
                [dst_blk_id = dst_blk.to_string(), dst_port](const graph_edge_t& edge) {
                    return edge.dst_blockid == dst_blk_id && edge.dst_port == dst_port;
                }),
            dst_blk.to_string());
        if (block_id_t(dst_static_edge.src_blockid).get_block_name() != NODE_ID_SEP) {
            const std::string err_msg =
                dst_blk.to_string() + ":" + std::to_string(dst_port)
                + " is not connected to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        // Now get the name and address of the SEP
        const std::string sep_block_id = dst_static_edge.src_blockid;
        const sep_addr_t sep_addr      = _sep_map.at(sep_block_id);

        const sw_buff_t pyld_fmt =
            bits_to_sw_buff(rfnoc_streamer->get_otw_item_comp_bit_width());
        const sw_buff_t mdata_fmt = BUFF_U64;

        auto xport = _gsm->create_host_to_device_data_stream(sep_addr,
            pyld_fmt,
            mdata_fmt,
            adapter_id,
            rfnoc_streamer->get_stream_args().args,
            rfnoc_streamer->get_unique_id());

        rfnoc_streamer->connect_channel(strm_port, std::move(xport));

        // If this worked, then also connect the streamer in the BGL graph
        auto dst = get_block(dst_blk);
        graph_edge_t edge_info(strm_port, dst_port, graph_edge_t::TX_STREAM, true);
        _graph->connect(rfnoc_streamer.get(), dst.get(), edge_info);

        _tx_streamers[rfnoc_streamer->get_unique_id()].node = rfnoc_streamer.get();
        _tx_streamers[rfnoc_streamer->get_unique_id()].connections[strm_port] = {
            rfnoc_streamer.get(), dst.get(), edge_info};
    }

    void connect(const block_id_t& src_blk,
        size_t src_port,
        uhd::rx_streamer::sptr streamer,
        size_t strm_port,
        uhd::transport::adapter_id_t adapter_id) override
    {
        // Verify the streamer was created by us
        auto rfnoc_streamer = std::dynamic_pointer_cast<rfnoc_rx_streamer>(streamer);
        if (!rfnoc_streamer) {
            throw uhd::type_error("Streamer is not rfnoc capable");
        }

        // Verify src_blk even exists in this graph
        if (!has_block(src_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect block to streamer, source block not found: ")
                + src_blk.to_string());
        }

        // Verify src_blk has an SEP downstream
        graph_edge_t src_static_edge = _assert_edge(
            _get_static_edge(
                [src_blk_id = src_blk.to_string(), src_port](const graph_edge_t& edge) {
                    return edge.src_blockid == src_blk_id && edge.src_port == src_port;
                }),
            src_blk.to_string());
        if (block_id_t(src_static_edge.dst_blockid).get_block_name() != NODE_ID_SEP) {
            const std::string err_msg =
                src_blk.to_string() + ":" + std::to_string(src_port)
                + " is not connected to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        // Now get the name and address of the SEP
        const std::string sep_block_id = src_static_edge.dst_blockid;
        const sep_addr_t sep_addr      = _sep_map.at(sep_block_id);

        const sw_buff_t pyld_fmt =
            bits_to_sw_buff(rfnoc_streamer->get_otw_item_comp_bit_width());
        const sw_buff_t mdata_fmt = BUFF_U64;

        auto xport = _gsm->create_device_to_host_data_stream(sep_addr,
            pyld_fmt,
            mdata_fmt,
            adapter_id,
            rfnoc_streamer->get_stream_args().args,
            rfnoc_streamer->get_unique_id());

        rfnoc_streamer->connect_channel(strm_port, std::move(xport));

        // If this worked, then also connect the streamer in the BGL graph
        auto src = get_block(src_blk);
        graph_edge_t edge_info(src_port, strm_port, graph_edge_t::RX_STREAM, true);
        _graph->connect(src.get(), rfnoc_streamer.get(), edge_info);

        _rx_streamers[rfnoc_streamer->get_unique_id()].node = rfnoc_streamer.get();
        _rx_streamers[rfnoc_streamer->get_unique_id()].connections[strm_port] = {
            src.get(), rfnoc_streamer.get(), edge_info};
    }

    void disconnect(const std::string& streamer_id) override
    {
        UHD_LOG_TRACE(LOG_ID, std::string("Disconnecting ") + streamer_id);
        if (_tx_streamers.count(streamer_id)) {
            // TODO: Physically disconnect all connections
            // This may not be strictly necessary because the destruction of
            // the xport will prevent packets from being sent to the
            // destination.

            // Remove the node from the graph
            _graph->remove(_tx_streamers[streamer_id].node);

            // Remove the streamer from the map
            _tx_streamers.erase(streamer_id);
        } else if (_rx_streamers.count(streamer_id)) {
            // TODO: Physically disconnect all connections

            // Remove the node from the graph (logically disconnect)
            _graph->remove(_rx_streamers[streamer_id].node);

            // Remove the streamer from the map
            _rx_streamers.erase(streamer_id);
        }
        UHD_LOG_TRACE(LOG_ID, std::string("Disconnected ") + streamer_id);
    }

    void disconnect(const std::string& streamer_id, size_t port) override
    {
        std::string id_str = streamer_id + ":" + std::to_string(port);
        UHD_LOG_TRACE(LOG_ID, std::string("Disconnecting ") + id_str);
        if (_tx_streamers.count(streamer_id)) {
            if (_tx_streamers[streamer_id].connections.count(port)) {
                auto connection = _tx_streamers[streamer_id].connections[port];
                _graph->disconnect(connection.src, connection.dst, connection.edge);
                _tx_streamers[streamer_id].connections.erase(port);
            } else {
                throw uhd::lookup_error(
                    std::string("Cannot disconnect. Port not connected: ") + id_str);
            }
        } else if (_rx_streamers.count(streamer_id)) {
            if (_rx_streamers[streamer_id].connections.count(port)) {
                auto connection = _rx_streamers[streamer_id].connections[port];
                // TODO: Physically disconnect port
                _graph->disconnect(connection.src, connection.dst, connection.edge);
                _rx_streamers[streamer_id].connections.erase(port);
            } else {
                throw uhd::lookup_error(
                    std::string("Cannot disconnect. Port not connected: ") + id_str);
            }
        }
        UHD_LOG_TRACE(LOG_ID, std::string("Disconnected ") + id_str);
    }

    uhd::rx_streamer::sptr create_rx_streamer(
        const size_t num_ports, const uhd::stream_args_t& args) override
    {
        auto this_graph = shared_from_this();
        return std::make_shared<rfnoc_rx_streamer>(
            num_ports, args, [this_graph](const std::string& id) { this_graph->disconnect(id); });
    }

    uhd::tx_streamer::sptr create_tx_streamer(
        const size_t num_ports, const uhd::stream_args_t& args) override
    {
        auto this_graph = shared_from_this();
        return std::make_shared<rfnoc_tx_streamer>(
            num_ports, args, [this_graph](const std::string& id) { this_graph->disconnect(id); });
    }

    size_t get_num_mboards() const override
    {
        return _num_mboards;
    }

    std::shared_ptr<mb_controller> get_mb_controller(const size_t mb_index = 0) override
    {
        if (_mb_controllers.size() <= mb_index) {
            throw uhd::index_error(
                std::string("Could not get mb controller for motherboard index ")
                + std::to_string(mb_index));
        }
        return _mb_controllers.at(mb_index);
    }

    bool synchronize_devices(const uhd::time_spec_t& time_spec, const bool quiet) override
    {
        auto mb_controllers_copy = _mb_controllers;
        bool result =
            _mb_controllers.at(0)->synchronize(mb_controllers_copy, time_spec, quiet);
        if (mb_controllers_copy.size() != _mb_controllers.size()) {
            // This shouldn't happen until we allow different device types in a
            // rfnoc_graph
            UHD_LOG_ERROR(LOG_ID, "Some devices wouldn't be sync'd!");
            return false;
        }
        return result;
    }

    uhd::property_tree::sptr get_tree(void) const override
    {
        return _tree;
    }

    std::vector<uhd::transport::adapter_id_t> enumerate_adapters_to_dst(
        const block_id_t& dst_blk, size_t dst_port) override
    {
        // Verify dst_blk even exists in this graph
        if (!has_block(dst_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect block to streamer, source block not found: ")
                + dst_blk.to_string());
        }

        // Verify dst_blk has an SEP upstream
        graph_edge_t dst_static_edge = _assert_edge(
            _get_static_edge(
                [dst_blk_id = dst_blk.to_string(), dst_port](const graph_edge_t& edge) {
                    return edge.dst_blockid == dst_blk_id && edge.dst_port == dst_port;
                }),
            dst_blk.to_string());
        if (block_id_t(dst_static_edge.src_blockid).get_block_name() != NODE_ID_SEP) {
            const std::string err_msg =
                dst_blk.to_string() + ":" + std::to_string(dst_port)
                + " is not connected to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        // Now get the name and address of the SEP
        const std::string sep_block_id = dst_static_edge.src_blockid;
        const sep_addr_t sep_addr      = _sep_map.at(sep_block_id);

        // Find links that can reach the SEP
        return _gsm->get_adapters(sep_addr);
    }

    std::vector<uhd::transport::adapter_id_t> enumerate_adapters_from_src(
        const block_id_t& src_blk, size_t src_port) override
    {
        // Verify src_blk even exists in this graph
        if (!has_block(src_blk)) {
            throw uhd::lookup_error(
                std::string("Cannot connect block to streamer, source block not found: ")
                + src_blk.to_string());
        }

        // Verify src_blk has an SEP downstream
        graph_edge_t src_static_edge = _assert_edge(
            _get_static_edge(
                [src_blk_id = src_blk.to_string(), src_port](const graph_edge_t& edge) {
                    return edge.src_blockid == src_blk_id && edge.src_port == src_port;
                }),
            src_blk.to_string());
        if (block_id_t(src_static_edge.dst_blockid).get_block_name() != NODE_ID_SEP) {
            const std::string err_msg =
                src_blk.to_string() + ":" + std::to_string(src_port)
                + " is not connected to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        // Now get the name and address of the SEP
        const std::string sep_block_id = src_static_edge.dst_blockid;
        const sep_addr_t sep_addr      = _sep_map.at(sep_block_id);

        // Find links that can reach the SEP
        return _gsm->get_adapters(sep_addr);
    }

    std::vector<graph_edge_t> enumerate_active_connections() override

    {
        return _graph->enumerate_edges();
    }

    std::vector<graph_edge_t> enumerate_static_connections() const override
    {
        return _static_edges;
    }

    void commit() override
    {
        _graph->commit();
    }

    void release() override
    {
        _graph->release();
    }

private:
    /**************************************************************************
     * Device Setup
     *************************************************************************/
    void _init_io_srv_mgr(const uhd::device_addr_t& dev_addr)
    {
        _io_srv_mgr = usrp::io_service_mgr::make(dev_addr);
        for (size_t mb_idx = 0; mb_idx < _num_mboards; mb_idx++) {
            _device->get_mb_iface(mb_idx).set_io_srv_mgr(_io_srv_mgr);
        }
    }

    void _init_mb_controllers()
    {
        UHD_LOG_TRACE(LOG_ID, "Initializing MB controllers...");
        for (size_t i = 0; i < _num_mboards; ++i) {
            _mb_controllers.push_back(_device->get_mb_controller(i));
        }
    }

    void _init_gsm()
    {
        UHD_LOG_TRACE(LOG_ID, "Initializing GSM...");
        auto e2s = [](uhd::endianness_t endianness) {
            return endianness == uhd::ENDIANNESS_BIG ? "BIG" : "LITTLE";
        };
        const chdr_w_t chdr_w              = _device->get_mb_iface(0).get_chdr_w();
        const uhd::endianness_t endianness = _device->get_mb_iface(0).get_endianness();
        for (size_t mb_idx = 1; mb_idx < _num_mboards; mb_idx++) {
            if (_device->get_mb_iface(mb_idx).get_chdr_w() != chdr_w) {
                throw uhd::runtime_error(
                    std::string("Non-homogenous devices: Graph CHDR width is ")
                    + std::to_string(chdr_w_to_bits(chdr_w)) + " but device "
                    + std::to_string(mb_idx) + " has CHDR width of "
                    + std::to_string(
                          chdr_w_to_bits(_device->get_mb_iface(mb_idx).get_chdr_w()))
                    + " bits!");
            }
            if (_device->get_mb_iface(mb_idx).get_endianness() != endianness) {
                throw uhd::runtime_error(
                    std::string("Non-homogenous devices: Graph endianness is ")
                    + e2s(endianness) + " but device " + std::to_string(mb_idx)
                    + " has endianness " + e2s(endianness) + "!");
            }
        }
        UHD_LOG_TRACE(LOG_ID,
            "Creating packet factory with CHDR width "
                << chdr_w_to_bits(chdr_w) << " bits and endianness " << e2s(endianness));
        _pkt_factory = std::make_unique<chdr::chdr_packet_factory>(chdr_w, endianness);
        // Create a collection of link definitions: (ID, MB) pairs
        std::vector<std::pair<device_id_t, mb_iface*>> links;
        for (size_t mb_idx = 0; mb_idx < _num_mboards; mb_idx++) {
            const auto device_ids = _device->get_mb_iface(mb_idx).get_local_device_ids();
            for (const device_id_t local_device_id : device_ids) {
                if (_device->get_mb_iface(mb_idx).get_endianness(local_device_id)
                    != endianness) {
                    throw uhd::runtime_error(
                        std::string("Non-homogenous devices: Graph endianness is ")
                        + e2s(endianness) + " but device " + std::to_string(mb_idx)
                        + " has endianness " + e2s(endianness) + "!");
                }
                links.push_back(
                    std::make_pair(local_device_id, &_device->get_mb_iface(mb_idx)));
            }
        }
        if (links.empty()) {
            UHD_LOG_ERROR(
                LOG_ID, "No links found for " << _num_mboards << " motherboards!");
            throw uhd::runtime_error("[rfnoc_graph] No links found!");
        }
        UHD_LOG_TRACE(LOG_ID, "Found a total of " << links.size() << " links.");
        try {
            _gsm = graph_stream_manager::make(*_pkt_factory, _epid_alloc, links);
        } catch (uhd::io_error& ex) {
            UHD_LOG_ERROR(LOG_ID, "IO Error during GSM initialization. " << ex.what());
            throw;
        }

        // Configure endpoint_manager, make sure all routes are established
        // FIXME
    }

    // Initialize client zero and all block controllers for motherboard mb_idx
    void _init_blocks(const size_t mb_idx, const uhd::device_addr_t& dev_addr)
    {
        UHD_LOG_TRACE(LOG_ID, "Initializing blocks for MB " << mb_idx << "...");
        // Setup the interfaces for this mboard and get some configuration info
        mb_iface& mb = _device->get_mb_iface(mb_idx);
        // Ask GSM to allow us to talk to our remote mb
        sep_addr_t ctrl_sep_addr(mb.get_remote_device_id(), 0);
        _gsm->connect_host_to_device(ctrl_sep_addr);
        // Grab and stash the Client Zero for this mboard
        detail::client_zero::sptr mb_cz = _gsm->get_client_zero(ctrl_sep_addr);
        // Client zero port numbers are based on the control xbar numbers,
        // which have the client 0 interface first, followed by stream
        // endpoints, and then the blocks.
        _client_zeros.emplace(mb_idx, mb_cz);

        const size_t num_blocks       = mb_cz->get_num_blocks();
        const size_t first_block_port = 1 + mb_cz->get_num_stream_endpoints();

        /* Flush and reset each block in the mboard
         * We do this before we enumerate the blocks to ensure they're in a clean
         * state before we construct their block controller, and so that we don't
         * reset any setting that the block controller writes
         */
        UHD_LOG_TRACE(LOG_ID,
            std::string("Flushing and resetting blocks on mboard ")
                + std::to_string(mb_idx));
        _flush_and_reset_mboard(mb_cz, num_blocks, first_block_port);

        // Make a map to count the number of each block we have
        std::unordered_map<std::string, uint16_t> block_count_map;

        // Iterate through and register each of the blocks in this mboard
        for (size_t portno = 0; portno < num_blocks; ++portno) {
            const auto noc_id       = mb_cz->get_noc_id(portno + first_block_port);
            const auto device_type  = mb_cz->get_device_type();
            auto block_factory_info = factory::get_block_factory(noc_id, device_type);
            auto block_info         = mb_cz->get_block_info(portno + first_block_port);
            block_id_t block_id(mb_idx,
                block_factory_info.block_name,
                block_count_map[block_factory_info.block_name]++);
            // Get access to the clock interface objects. We have some rules
            // here:
            // - The ctrlport clock must always be provided through the
            //   BSP via mb_iface
            // - The timebase clock can be set to CLOCK_KEY_GRAPH, which means
            //   the block takes care of the timebase itself (via property
            //   propagation). In that case, we generate a clock iface
            //   object on the fly here.
            // - In all other cases, the BSP must provide us that clock
            //   iface object through the mb_iface
            auto ctrlport_clk_iface = mb.get_clock_iface(block_factory_info.ctrlport_clk);
            auto tb_clk_iface       = (block_factory_info.timebase_clk == CLOCK_KEY_GRAPH)
                                    ? std::make_shared<clock_iface>(CLOCK_KEY_GRAPH)
                                    : mb.get_clock_iface(block_factory_info.timebase_clk);
            // A "graph" clock is always "running"
            if (block_factory_info.timebase_clk == CLOCK_KEY_GRAPH) {
                tb_clk_iface->set_running(true);
            }
            auto block_reg_iface = _gsm->get_block_register_iface(
                ctrl_sep_addr, portno, *ctrlport_clk_iface.get(), *tb_clk_iface.get());
            auto make_args_uptr      = std::make_unique<noc_block_base::make_args_t>();
            make_args_uptr->noc_id   = noc_id;
            make_args_uptr->block_id = block_id;
            make_args_uptr->num_input_ports  = block_info.num_inputs;
            make_args_uptr->num_output_ports = block_info.num_outputs;
            make_args_uptr->mtu =
                (1 << block_info.data_mtu) * chdr_w_to_bits(mb.get_chdr_w()) / 8;
            make_args_uptr->chdr_w             = mb.get_chdr_w();
            make_args_uptr->reg_iface          = block_reg_iface;
            make_args_uptr->tb_clk_iface       = tb_clk_iface;
            make_args_uptr->ctrlport_clk_iface = ctrlport_clk_iface;
            make_args_uptr->mb_control =
                block_factory_info.mb_access ? _mb_controllers.at(mb_idx) : nullptr;
            const uhd::fs_path block_path(uhd::fs_path("/blocks") / block_id.to_string());
            _tree->create<uint32_t>(block_path / "noc_id").set(noc_id);
            make_args_uptr->tree = _tree->subtree(block_path);
            make_args_uptr->args = dev_addr; // TODO filter the device args
            try {
                _block_registry->register_block(
                    block_factory_info.factory_fn(std::move(make_args_uptr)));
                block_initializer::post_init(_block_registry->get_block(block_id));
            } catch (...) {
                UHD_LOG_ERROR(
                    LOG_ID, "Error during initialization of block " << block_id << "!");
                throw;
            }
            _xbar_block_config[block_id.to_string()] = {
                portno, noc_id, block_id.get_block_count()};

            _port_block_map.insert({{mb_idx, portno + first_block_port}, block_id});
        }
    }

    void _init_sep_map()
    {
        for (size_t mb_idx = 0; mb_idx < _num_mboards; ++mb_idx) {
            auto remote_device_id = _device->get_mb_iface(mb_idx).get_remote_device_id();
            auto& cz              = _client_zeros.at(mb_idx);
            for (size_t sep_idx = 0; sep_idx < cz->get_num_stream_endpoints();
                 ++sep_idx) {
                // Register ID in _port_block_map
                block_id_t id(mb_idx, NODE_ID_SEP, sep_idx);
                _port_block_map.insert({{mb_idx, sep_idx + 1}, id});
                _sep_map.insert({id.to_string(), sep_addr_t(remote_device_id, sep_idx)});
            }
        }
    }

    void _init_static_connections()
    {
        UHD_LOG_TRACE(LOG_ID, "Identifying static connections...");
        for (auto& kv_cz : _client_zeros) {
            auto& adjacency_list = kv_cz.second->get_adjacency_list();
            for (auto& edge : adjacency_list) {
                // Assemble edge
                auto graph_edge = graph_edge_t();
                UHD_ASSERT_THROW(
                    _port_block_map.count({kv_cz.first, edge.src_blk_index}));
                graph_edge.src_blockid =
                    _port_block_map.at({kv_cz.first, edge.src_blk_index});
                UHD_ASSERT_THROW(
                    _port_block_map.count({kv_cz.first, edge.dst_blk_index}));
                graph_edge.dst_blockid =
                    _port_block_map.at({kv_cz.first, edge.dst_blk_index});
                graph_edge.src_port = edge.src_blk_port;
                graph_edge.dst_port = edge.dst_blk_port;
                graph_edge.edge     = graph_edge_t::edge_t::STATIC;
                _static_edges.push_back(graph_edge);
                UHD_LOG_TRACE(LOG_ID, "Static connection: " << graph_edge.to_string());
            }
        }
    }

    //! Initialize the motherboard controllers, if they require it
    void _init_mbc()
    {
        for (size_t i = 0; i < _mb_controllers.size(); ++i) {
            UHD_LOG_TRACE(LOG_ID, "Calling MBC init for motherboard " << i);
            _mb_controllers.at(i)->init();
        }
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
        graph_edge_t::edge_t edge_type,
        bool is_back_edge)
    {
        graph_edge_t edge_info(
            src_port, dst_port, edge_type, not is_back_edge);
        edge_info.src_blockid = src_blk->get_unique_id();
        edge_info.dst_blockid = dst_blk->get_unique_id();
        _graph->connect(src_blk.get(), dst_blk.get(), edge_info);
    }

    /*! Internal helper to get route information
     *
     * Checks the validity of the route and returns route information.
     *
     * \throws uhd::routing_error
     *     if the routing is impossible
     */
    route_info_t _get_route_info(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port)
    {
        graph_edge_t::edge_t edge_type = graph_edge_t::DYNAMIC;

        const std::string src_blk_info =
            src_blk.to_string() + ":" + std::to_string(src_port);
        const std::string dst_blk_info =
            dst_blk.to_string() + ":" + std::to_string(dst_port);

        // Find the static edge for src_blk:src_port
        auto src_static_edge = _assert_edge(
            _get_static_edge(
                [src_blk_id = src_blk.to_string(), src_port](const graph_edge_t& edge) {
                    return edge.src_blockid == src_blk_id && edge.src_port == src_port;
                }),
            src_blk_info);

        // Now find the static edge for the destination SEP
        auto dst_static_edge = _assert_edge(
            _get_static_edge(
                [dst_blk_id = dst_blk.to_string(), dst_port](const graph_edge_t& edge) {
                    return edge.dst_blockid == dst_blk_id && edge.dst_port == dst_port;
                }),
            dst_blk_info);

        // Now see if it's already connected to the destination
        if (src_static_edge.dst_blockid == dst_blk.to_string()
            && src_static_edge.dst_port == dst_port) {
            // Blocks are statically connected
            UHD_LOG_TRACE(LOG_ID,
                "Blocks " << src_blk_info << " and " << dst_blk_info
                          << " are statically connected");
            edge_type = graph_edge_t::STATIC;
        } else if (block_id_t(src_static_edge.dst_blockid).get_block_name()
                   != NODE_ID_SEP) {
            // Blocks are not statically connected and the source is not
            // connected to an SEP
            const std::string err_msg =
                src_blk_info + " is neither statically connected to " + dst_blk_info
                + " nor to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        } else if (block_id_t(dst_static_edge.src_blockid).get_block_name()
                   != NODE_ID_SEP) {
            // Blocks are not statically connected and the destination is not
            // connected to an SEP
            const std::string err_msg =
                dst_blk_info + " is neither statically connected to " + src_blk_info
                + " nor to an SEP! Routing impossible.";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }

        return {edge_type, src_static_edge, dst_static_edge};
    }

    /*! Internal physical connection helper
     *
     * Make the connections in the physical device
     *
     * \throws uhd::routing_error
     *     if the routing is impossible
     */
    graph_edge_t::edge_t _physical_connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port)
    {
        auto route_info = _get_route_info(src_blk, src_port, dst_blk, dst_port);

        if (route_info.edge_type == graph_edge_t::DYNAMIC) {
            const std::string src_sep_info = route_info.src_static_edge.dst_blockid;
            const sep_addr_t src_sep_addr  = _sep_map.at(src_sep_info);
            const std::string dst_sep_info = route_info.dst_static_edge.src_blockid;
            const sep_addr_t dst_sep_addr  = _sep_map.at(dst_sep_info);

            auto strm_info = _gsm->create_device_to_device_data_stream(
                dst_sep_addr, src_sep_addr, false, 0.1, 0.0, false);

            UHD_LOGGER_DEBUG(LOG_ID)
                << boost::format(
                       "Data stream between EPID %d and EPID %d established "
                       "where downstream buffer can hold %lu bytes and %u packets")
                       % std::get<0>(strm_info).first % std::get<0>(strm_info).second
                       % std::get<1>(strm_info).bytes % std::get<1>(strm_info).packets;
        }

        return route_info.edge_type;
    }

    /*! Internal physical disconnection helper
     *
     * Disconnects the physical device
     *
     * \throws uhd::routing_error
     *     if the routing is impossible
     */
    graph_edge_t::edge_t _physical_disconnect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port)
    {
        auto route_info = _get_route_info(src_blk, src_port, dst_blk, dst_port);

        if (route_info.edge_type == graph_edge_t::DYNAMIC) {
            // TODO: Add call into _gsm to physically disconnect the SEPs
        }

        return route_info.edge_type;
    }

    //! Flush and reset each connected port on the mboard
    void _flush_and_reset_mboard(detail::client_zero::sptr mb_cz,
        const size_t num_blocks,
        const size_t first_block_port)
    {
        if (!mb_cz->complete_flush_all_blocks()) {
            UHD_LOG_WARNING(LOG_ID, "One or more blocks timed out during flush!");
        }

        // Reset
        for (size_t portno = 0; portno < num_blocks; ++portno) {
            auto block_portno = portno + first_block_port;
            mb_cz->reset_chdr(block_portno);
            mb_cz->reset_ctrl(block_portno);
        }
    }

    /*! Find the static edge that matches \p pred
     *
     * \throws uhd::assertion_error if the edge can't be found. So be careful!
     */
    template <typename UnaryPredicate>
    boost::optional<graph_edge_t> _get_static_edge(UnaryPredicate&& pred)
    {
        auto edge_it = std::find_if(_static_edges.cbegin(), _static_edges.cend(), pred);
        if (edge_it == _static_edges.cend()) {
            return boost::none;
        }
        return *edge_it;
    }

    /*! Make sure an optional edge info is valid, or throw.
     */
    graph_edge_t _assert_edge(
        boost::optional<graph_edge_t> edge_o, const std::string& blk_info)
    {
        if (!bool(edge_o)) {
            const std::string err_msg = std::string("Cannot connect block ") + blk_info
                                        + ", port is unconnected in the FPGA!";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::routing_error(err_msg);
        }
        return edge_o.get();
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to the underlying device implementation
    detail::rfnoc_device::sptr _device;

    //! Reference to the property tree
    uhd::property_tree::sptr _tree;

    //! Number of motherboards, this is technically redundant but useful for
    // easy lookups.
    size_t _num_mboards;

    //! Reference to the global I/O Service Manager
    uhd::usrp::io_service_mgr::sptr _io_srv_mgr;

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
    std::vector<mb_controller::sptr> _mb_controllers;

    //! Stash of the client zeros for all motherboards
    std::unordered_map<size_t, detail::client_zero::sptr> _client_zeros;

    //! Map a pair (motherboard index, control crossbar port) to an RFNoC block
    // or SEP
    std::map<std::pair<size_t, size_t>, block_id_t> _port_block_map;

    //! Map SEP block ID (e.g. 0/SEP#0) onto a sep_addr_t
    std::unordered_map<std::string, sep_addr_t> _sep_map;

    //! List of statically connected edges. Includes SEPs too!
    std::vector<graph_edge_t> _static_edges;

    //! uptr to graph stream manager
    graph_stream_manager::uptr _gsm;

    //! EPID allocator. Technically not required by the rfnoc_graph, but we'll
    // store it here because it's such a central thing.
    epid_allocator::sptr _epid_alloc = std::make_shared<epid_allocator>();

    //! Reference to a packet factory object. Gets initialized just before the GSM
    std::unique_ptr<chdr::chdr_packet_factory> _pkt_factory;

    //! Map from TX streamer ID to streamer info
    std::map<std::string, streamer_info_t> _tx_streamers;

    //! Map from RX streamer ID to streamer info
    std::map<std::string, streamer_info_t> _rx_streamers;
}; /* class rfnoc_graph_impl */


/******************************************************************************
 * Factory
 *****************************************************************************/
namespace uhd { namespace rfnoc { namespace detail {

// Simple factory: If we already have the device, simply pass it on
rfnoc_graph::sptr make_rfnoc_graph(
    detail::rfnoc_device::sptr dev, const uhd::device_addr_t& device_addr)
{
    static std::mutex _map_mutex;
    static std::map<std::weak_ptr<rfnoc_device>,
        std::weak_ptr<rfnoc_graph>,
        std::owner_less<std::weak_ptr<rfnoc_device>>>
        dev_to_graph;
    rfnoc_graph::sptr graph;

    // Check if a graph was already created for this device
    std::lock_guard<std::mutex> lock(_map_mutex);
    if (dev_to_graph.count(dev) and not dev_to_graph[dev].expired()) {
        graph = dev_to_graph[dev].lock();
        if (graph != nullptr) {
            return graph;
        }
    }

    // Create a new graph
    graph             = std::make_shared<rfnoc_graph_impl>(dev, device_addr);
    dev_to_graph[dev] = graph;
    return graph;
}

}}} /* namespace uhd::rfnoc::detail */

// If we don't have a device yet, create it and see if it's really an RFNoC
// device. This is used by multi_usrp_rfnoc, for example.
rfnoc_graph::sptr rfnoc_graph::make(const uhd::device_addr_t& device_addr)
{
    auto dev =
        std::dynamic_pointer_cast<detail::rfnoc_device>(uhd::device::make(device_addr));
    if (!dev) {
        throw uhd::key_error(std::string("No RFNoC devices found for ----->\n")
                             + device_addr.to_pp_string());
    }
    return std::make_shared<rfnoc_graph_impl>(dev, device_addr);
}
