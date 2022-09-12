//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph_stream_manager.hpp>
#include <uhdlib/rfnoc/link_stream_manager.hpp>
#include <uhdlib/transport/links.hpp>
#include <boost/format.hpp>
#include <map>
#include <memory>
#include <set>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

graph_stream_manager::~graph_stream_manager() = default;

class graph_stream_manager_impl : public graph_stream_manager
{
public:
    graph_stream_manager_impl(const chdr::chdr_packet_factory& pkt_factory,
        const epid_allocator::sptr& epid_alloc,
        const std::vector<std::pair<device_id_t, mb_iface*>>& links)
        : _epid_alloc(epid_alloc)
    {
        for (const auto& lnk : links) {
            UHD_ASSERT_THROW(lnk.second);
            _link_mgrs.emplace(lnk.first,
                link_stream_manager::make(
                    pkt_factory, *lnk.second, epid_alloc, lnk.first));
            auto adapter = _link_mgrs.at(lnk.first)->get_adapter_id();
            if (_alloc_map.count(adapter) == 0) {
                _alloc_map[adapter] = allocation_info{0, 0};
            }
        }
        for (const auto& mgr_pair : _link_mgrs) {
            for (const auto& ep : mgr_pair.second->get_reachable_endpoints()) {
                // Add the (potential) destinations to the
                _reachable_endpoints.insert(ep);
                // Add entry to source map
                if (_src_map.count(ep) == 0) {
                    _src_map[ep] = std::vector<device_id_t>();
                }
                _src_map[ep].push_back(mgr_pair.first);
            }
        }
    }

    ~graph_stream_manager_impl() override = default;

    const std::set<sep_addr_t>& get_reachable_endpoints() const override
    {
        return _reachable_endpoints;
    }

    std::vector<device_id_t> get_local_devices() const override
    {
        std::vector<device_id_t> retval;
        for (const auto& mgr_pair : _link_mgrs) {
            retval.push_back(mgr_pair.first);
        }
        return retval;
    }

    sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr,
        uhd::transport::adapter_id_t adapter = uhd::transport::NULL_ADAPTER_ID) override
    {
        UHD_LOGGER_DEBUG("RFNOC::GRAPH")
            << boost::format("Connecting the Host to Endpoint %d:%d through Adapter "
                             "%d (0 = no preference)... ")
                   % dst_addr.first % dst_addr.second % adapter;

        // When we connect, we setup a route and fire up a control stream between
        // the endpoints
        device_id_t gateway =
            _check_dst_and_find_src(dst_addr, adapter, uhd::transport::link_type_t::CTRL);
        sep_id_pair_t epid_pair =
            _link_mgrs.at(gateway)->connect_host_to_device(dst_addr);
        UHD_LOGGER_DEBUG("RFNOC::GRAPH")
            << boost::format("Connection to Endpoint %d:%d completed through Device %d. "
                             "Using EPIDs %d -> %d.")
                   % dst_addr.first % dst_addr.second % gateway % epid_pair.first
                   % epid_pair.second;

        return epid_pair;
    }

    sep_id_pair_t connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) override
    {
        UHD_LOGGER_DEBUG("RFNOC::GRAPH")
            << boost::format("Connecting the Endpoint %d:%d to Endpoint %d:%d...")
                   % src_addr.first % src_addr.second % dst_addr.first % dst_addr.second;

        // Iterate through all link managers and check if they are capable of connecting
        // the requested endpoints. If no one can connect then the endpoints may actually
        // not share a common crossbar or we don't have enough connectivity in the
        // software session to reach the common crossbar.
        for (auto& kv : _link_mgrs) {
            if (kv.second->can_connect_device_to_device(dst_addr, src_addr)) {
                sep_id_pair_t epid_pair =
                    kv.second->connect_device_to_device(dst_addr, src_addr);
                UHD_LOGGER_DEBUG("RFNOC::GRAPH")
                    << boost::format("Connection from Endpoint %d:%d to Endpoint %d:%d "
                                     "completed through Device %d. Using "
                                     "EPIDs %d -> %d.")
                           % src_addr.first % src_addr.second % dst_addr.first
                           % dst_addr.second % kv.first % epid_pair.first
                           % epid_pair.second;
                return epid_pair;
            }
        }
        throw uhd::routing_error("The specified destination is unreachable from the "
                                 "specified source endpoint");
    }

    ctrlport_endpoint::sptr get_block_register_iface(sep_addr_t dst_addr,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk,
        uhd::transport::adapter_id_t adapter = uhd::transport::NULL_ADAPTER_ID) override
    {
        // We must be connected to dst_addr before getting a register iface
        sep_id_t dst_epid = _epid_alloc->get_epid(dst_addr);
        auto dev =
            _check_dst_and_find_src(dst_addr, adapter, uhd::transport::link_type_t::CTRL);
        return _link_mgrs.at(dev)->get_block_register_iface(
            dst_epid, block_index, client_clk, timebase_clk);
    }

    detail::client_zero::sptr get_client_zero(sep_addr_t dst_addr,
        uhd::transport::adapter_id_t adapter =
            uhd::transport::NULL_ADAPTER_ID) const override
    {
        // We must be connected to dst_addr before getting a client zero
        sep_id_t dst_epid = _epid_alloc->get_epid(dst_addr);
        auto dev =
            _check_dst_and_find_src(dst_addr, adapter, uhd::transport::link_type_t::CTRL);
        return _link_mgrs.at(dev)->get_client_zero(dst_epid);
    }

    std::tuple<sep_id_pair_t, stream_buff_params_t> create_device_to_device_data_stream(
        const sep_addr_t dst_addr,
        const sep_addr_t src_addr,
        const bool lossy_xport,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const bool reset = false) override
    {
        UHD_LOGGER_DEBUG("RFNOC::GRAPH")
            << boost::format(
                   "Initializing data stream from Endpoint %d:%d to Endpoint %d:%d...")
                   % src_addr.first % src_addr.second % dst_addr.first % dst_addr.second;

        // Iterate through all link managers and check if they are capable of connecting
        // the requested endpoints. If no one can connect then the endpoints may actually
        // not share a common crossbar or we don't have enough connectivity in the
        // software session to reach the common crossbar.
        for (auto& kv : _link_mgrs) {
            if (kv.second->can_connect_device_to_device(dst_addr, src_addr)) {
                sep_id_pair_t epid_pair =
                    kv.second->connect_device_to_device(dst_addr, src_addr);
                UHD_LOGGER_DEBUG("RFNOC::GRAPH")
                    << boost::format("Connection from Endpoint %d:%d to Endpoint %d:%d "
                                     "completed through Device %d. Using "
                                     "EPIDs %d -> %d.")
                           % src_addr.first % src_addr.second % dst_addr.first
                           % dst_addr.second % kv.first % epid_pair.first
                           % epid_pair.second;
                stream_buff_params_t buff_params =
                    kv.second->create_device_to_device_data_stream(epid_pair.second,
                        epid_pair.first,
                        lossy_xport,
                        fc_freq_ratio,
                        fc_headroom_ratio,
                        reset);
                return std::make_tuple(epid_pair, buff_params);
            }
        }
        throw uhd::routing_error("The specified destination is unreachable from the "
                                 "specified source endpoint");
    }

    chdr_rx_data_xport::uptr create_device_to_host_data_stream(const sep_addr_t src_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const uhd::transport::adapter_id_t adapter,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        device_id_t dev = _check_dst_and_find_src(
            src_addr, adapter, uhd::transport::link_type_t::RX_DATA);
        uhd::transport::adapter_id_t chosen = _link_mgrs.at(dev)->get_adapter_id();
        auto allocs                         = _alloc_map.at(chosen);
        allocs.rx++;
        _alloc_map[chosen] = allocs;
        return _link_mgrs.at(dev)->create_device_to_host_data_stream(
            src_addr, pyld_buff_fmt, mdata_buff_fmt, xport_args, streamer_id);
    }

    chdr_tx_data_xport::uptr create_host_to_device_data_stream(sep_addr_t dst_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const uhd::transport::adapter_id_t adapter,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        device_id_t dev = _check_dst_and_find_src(
            dst_addr, adapter, uhd::transport::link_type_t::TX_DATA);
        uhd::transport::adapter_id_t chosen = _link_mgrs.at(dev)->get_adapter_id();
        auto allocs                         = _alloc_map.at(chosen);
        allocs.tx++;
        _alloc_map[chosen] = allocs;
        return _link_mgrs.at(dev)->create_host_to_device_data_stream(
            dst_addr, pyld_buff_fmt, mdata_buff_fmt, xport_args, streamer_id);
    }

    std::vector<uhd::transport::adapter_id_t> get_adapters(sep_addr_t addr) const override
    {
        auto adapters = std::vector<uhd::transport::adapter_id_t>();
        if (_src_map.count(addr) > 0) {
            const auto& src_devs = _src_map.at(addr);
            for (auto src : src_devs) {
                // Returns in order specified in device args
                // Assumption: the device_id_t will be incremented sequentially
                // and the std::map will then provide the link_stream_managers
                // in the same order as the adapters were specified
                adapters.push_back(_link_mgrs.at(src)->get_adapter_id());
            }
            return adapters;
        } else {
            throw uhd::rfnoc_error("Specified address is unreachable. No via_devices.");
        }
    }

private:
    //! Return the local device ID over which we can reach a destination
    //
    // \param dst_addr The destination address (device/instance pair) for which
    //                 we are finding a local device ID
    // \param adapter If provided (i.e., if not NULL_ADAPTER_ID) then only this
    //                adapter index is used to find local device IDs. If it is
    //                not given (i.e. if equal to NULL_ADAPTER_ID), then this
    //                function will use heuristics to choose an adapter.
    // \param link_type The type of link for which we're finding a local device
    //                  ID. When \p adapter is NULL_ADAPTER_ID, then we use this
    //                  in our heuristics for choosing an adapter.
    device_id_t _check_dst_and_find_src(sep_addr_t dst_addr,
        uhd::transport::adapter_id_t adapter,
        uhd::transport::link_type_t link_type) const
    {
        if (_src_map.count(dst_addr) > 0) {
            const auto& src_devs = _src_map.at(dst_addr);
            if (adapter == uhd::transport::NULL_ADAPTER_ID) {
                // TODO: Maybe we can come up with a better heuristic for when the user
                // gives no preference
                auto dev       = src_devs[0];
                auto dev_alloc = _alloc_map.at(_link_mgrs.at(dev)->get_adapter_id());
                for (auto candidate : src_devs) {
                    auto candidate_alloc =
                        _alloc_map.at(_link_mgrs.at(candidate)->get_adapter_id());
                    switch (link_type) {
                        case uhd::transport::link_type_t::TX_DATA:
                            if (candidate_alloc.tx < dev_alloc.tx) {
                                dev       = candidate;
                                dev_alloc = candidate_alloc;
                            }
                            break;
                        case uhd::transport::link_type_t::RX_DATA:
                            if (candidate_alloc.rx < dev_alloc.rx) {
                                dev       = candidate;
                                dev_alloc = candidate_alloc;
                            }
                            break;
                        default:
                            // Just accept first device for CTRL and ASYNC_MSG
                            break;
                    }
                }
                return dev;
            } else {
                for (const auto& src : src_devs) {
                    if (_link_mgrs.at(src)->get_adapter_id() == adapter) {
                        return src;
                    }
                }
                throw uhd::rfnoc_error("Specified destination address is unreachable "
                                       "from the via device");
            }
        } else {
            throw uhd::rfnoc_error("Specified destination address is unreachable");
        }
    }

    // The cached EPID allocator object
    epid_allocator::sptr _epid_alloc;
    // A map the contains all link manager indexed by the device ID
    std::map<device_id_t, link_stream_manager::uptr> _link_mgrs;
    // A set of the addresses of all devices reachable from this graph
    std::set<sep_addr_t> _reachable_endpoints;
    // A map of addresses that can be taken to reach a particular destination
    std::map<sep_addr_t, std::vector<device_id_t>> _src_map;

    // Data used for heuristic to determine which link to use
    struct allocation_info
    {
        size_t rx;
        size_t tx;
    };

    // A map of allocations for each host transport adapter
    std::map<uhd::transport::adapter_id_t, allocation_info> _alloc_map;
};

graph_stream_manager::uptr graph_stream_manager::make(
    const chdr::chdr_packet_factory& pkt_factory,
    const epid_allocator::sptr& epid_alloc,
    const std::vector<std::pair<device_id_t, mb_iface*>>& links)
{
    return std::make_unique<graph_stream_manager_impl>(pkt_factory, epid_alloc, links);
}
