//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph_stream_manager.hpp>
#include <uhdlib/rfnoc/link_stream_manager.hpp>
#include <boost/format.hpp>
#include <map>
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
                std::move(link_stream_manager::make(
                    pkt_factory, *lnk.second, epid_alloc, lnk.first)));
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

    virtual ~graph_stream_manager_impl() = default;

    virtual const std::set<sep_addr_t>& get_reachable_endpoints() const
    {
        return _reachable_endpoints;
    }

    virtual std::vector<device_id_t> get_local_devices() const
    {
        std::vector<device_id_t> retval;
        for (const auto& mgr_pair : _link_mgrs) {
            retval.push_back(mgr_pair.first);
        }
        return retval;
    }

    virtual sep_id_pair_t init_ctrl_stream(
        sep_addr_t dst_addr, device_id_t via_device = NULL_DEVICE_ID)
    {
        return _link_mgrs.at(_check_dst_and_find_src(dst_addr, via_device))
            ->init_ctrl_stream(dst_addr);
    }

    virtual ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk,
        device_id_t via_device = NULL_DEVICE_ID)
    {
        sep_addr_t dst_addr = _epid_alloc->lookup_epid(dst_epid);
        return _link_mgrs.at(_check_dst_and_find_src(dst_addr, via_device))
            ->get_block_register_iface(dst_epid, block_index, client_clk, timebase_clk);
    }

    virtual detail::client_zero::sptr get_client_zero(
        sep_id_t dst_epid, device_id_t via_device = NULL_DEVICE_ID) const
    {
        sep_addr_t dst_addr = _epid_alloc->lookup_epid(dst_epid);
        return _link_mgrs.at(_check_dst_and_find_src(dst_addr, via_device))
            ->get_client_zero(dst_epid);
    }

private:
    device_id_t _check_dst_and_find_src(sep_addr_t dst_addr, device_id_t via_device) const
    {
        if (_src_map.count(dst_addr) > 0) {
            const auto& src_devs = _src_map.at(dst_addr);
            if (via_device == NULL_DEVICE_ID) {
                // TODO: Maybe we can come up with a better heuristic for when the user
                // gives no preference
                return src_devs[0];
            } else {
                for (const auto& src : src_devs) {
                    if (src == via_device) {
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
};

graph_stream_manager::uptr graph_stream_manager::make(
    const chdr::chdr_packet_factory& pkt_factory,
    const epid_allocator::sptr& epid_alloc,
    const std::vector<std::pair<device_id_t, mb_iface*>>& links)
{
    return std::make_unique<graph_stream_manager_impl>(pkt_factory, epid_alloc, links);
}
