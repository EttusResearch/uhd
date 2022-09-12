//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <uhdlib/rfnoc/link_stream_manager.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <boost/format.hpp>
#include <map>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::transport;
using namespace uhd::rfnoc::chdr;
using namespace uhd::rfnoc::mgmt;
using namespace uhd::rfnoc::detail;

constexpr sep_inst_t SEP_INST_MGMT_CTRL = 0;
constexpr sep_inst_t SEP_INST_DATA_BASE = 1;

constexpr double STREAM_SETUP_TIMEOUT = 0.2;

link_stream_manager::~link_stream_manager() = default;

class link_stream_manager_impl : public link_stream_manager
{
public:
    link_stream_manager_impl(const chdr::chdr_packet_factory& pkt_factory,
        mb_iface& mb_if,
        const epid_allocator::sptr& epid_alloc,
        device_id_t device_id)
        : _pkt_factory(pkt_factory)
        , _my_device_id(device_id)
        , _mb_iface(mb_if)
        , _epid_alloc(epid_alloc)
        , _data_ep_inst(0)
    {
        // Sanity check if we can access our device ID from this motherboard
        const auto& mb_devs = _mb_iface.get_local_device_ids();
        if (std::find(mb_devs.begin(), mb_devs.end(), _my_device_id) == mb_devs.end()) {
            throw uhd::rfnoc_error("The device bound to this link manager cannot be "
                                   "accessed from this motherboard");
        }

        // Sanity check the protocol version and CHDR width
        if ((_pkt_factory.get_protover() & 0xFF00)
            != (_mb_iface.get_proto_ver() & 0xFF00)) {
            throw uhd::rfnoc_error("RFNoC protocol mismatch between SW and HW");
        }
        if (_pkt_factory.get_chdr_w() != _mb_iface.get_chdr_w()) {
            throw uhd::rfnoc_error("RFNoC CHDR width mismatch between SW and HW");
        }

        // Create a transport and EPID for management and control traffic
        _my_mgmt_ctrl_epid =
            epid_alloc->allocate_epid(sep_addr_t(_my_device_id, SEP_INST_MGMT_CTRL));
        _allocated_epids.insert(_my_mgmt_ctrl_epid);

        // Create a muxed transport to share between the mgmt_portal and
        // chdr_ctrl_endpoint. We have to use the same base transport here to ensure that
        // the route setup logic in the FPGA transport works correctly.
        _ctrl_xport = _mb_iface.make_ctrl_transport(_my_device_id, _my_mgmt_ctrl_epid);

        _my_adapter_id = _mb_iface.get_adapter_id(_my_device_id);

        // Create management portal using one of the child transports
        _mgmt_portal = mgmt_portal::make(
            *_ctrl_xport, _pkt_factory, sep_addr_t(_my_device_id, SEP_INST_MGMT_CTRL));
    }

    ~link_stream_manager_impl() override
    {
        for (const auto& epid : _allocated_epids) {
            _epid_alloc->deallocate_epid(epid);
        }
    }

    device_id_t get_self_device_id() const override
    {
        return _my_device_id;
    }

    uhd::transport::adapter_id_t get_adapter_id() const override
    {
        return _my_adapter_id;
    }

    const std::set<sep_addr_t>& get_reachable_endpoints() const override
    {
        return _mgmt_portal->get_reachable_endpoints();
    }

    bool can_connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) const override
    {
        return _mgmt_portal->can_remote_route(dst_addr, src_addr);
    }

    sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr) override
    {
        _ensure_ep_is_reachable(dst_addr);

        // Allocate EPIDs
        sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);

        // Make sure that the software side of the endpoint is initialized and reachable
        if (_ctrl_ep == nullptr) {
            // Create a control endpoint with that xport
            _ctrl_ep =
                chdr_ctrl_endpoint::make(_ctrl_xport, _pkt_factory, _my_mgmt_ctrl_epid);
        }

        // Setup a route to the EPID
        _mgmt_portal->setup_local_route(*_ctrl_xport, dst_epid);
        if (!_mgmt_portal->get_endpoint_info(dst_epid).has_ctrl) {
            throw uhd::rfnoc_error(
                "Downstream endpoint does not support control traffic");
        }

        // Create a client zero instance
        if (_client_zero_map.count(dst_epid) == 0) {
            _client_zero_map.insert(
                std::make_pair(dst_epid, client_zero::make(*_ctrl_ep, dst_epid)));
        }
        return sep_id_pair_t(_my_mgmt_ctrl_epid, dst_epid);
    }

    sep_id_pair_t connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) override
    {
        _ensure_ep_is_reachable(dst_addr);
        _ensure_ep_is_reachable(src_addr);

        // Allocate EPIDs and initialize endpoints
        sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);
        sep_id_t src_epid =
            _epid_alloc->allocate_epid(src_addr, *_mgmt_portal, *_ctrl_xport);

        // Set up routes
        _mgmt_portal->setup_remote_route(*_ctrl_xport, dst_epid, src_epid);

        return sep_id_pair_t(src_epid, dst_epid);
    }

    ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk) override
    {
        // Ensure that the endpoint is initialized for control at the specified EPID
        if (_ctrl_ep == nullptr) {
            throw uhd::runtime_error("Software endpoint not initialized for control");
        }
        if (_client_zero_map.count(dst_epid) == 0) {
            throw uhd::runtime_error(
                "Control for the specified EPID was not initialized");
        }
        const client_zero::sptr& c0_ctrl = _client_zero_map.at(dst_epid);
        const uint16_t block_slot = 1 + c0_ctrl->get_num_stream_endpoints() + block_index;
        if (block_index >= c0_ctrl->get_num_blocks()) {
            throw uhd::value_error("Requested block index out of range");
        }

        // Create control endpoint
        return _ctrl_ep->get_ctrlport_ep(dst_epid,
            c0_ctrl->get_ctrl_xbar_port(block_index),
            (size_t(1) << c0_ctrl->get_block_info(block_slot).ctrl_fifo_size),
            c0_ctrl->get_block_info(block_slot).ctrl_max_async_msgs,
            client_clk,
            timebase_clk);
    }

    client_zero::sptr get_client_zero(sep_id_t dst_epid) const override
    {
        if (_client_zero_map.count(dst_epid) == 0) {
            throw uhd::runtime_error(
                "Control for the specified EPID was not initialized");
        }
        return _client_zero_map.at(dst_epid);
    }

    stream_buff_params_t create_device_to_device_data_stream(const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const bool reset = false) override
    {
        // We assume that the devices are already connected (because this API requires
        // EPIDs)

        // Setup a stream
        stream_buff_params_t buff_params =
            _mgmt_portal->config_remote_stream(*_ctrl_xport,
                dst_epid,
                src_epid,
                lossy_xport,
                stream_buff_params_t{1, 1}, // Dummy frequency
                stream_buff_params_t{0, 0}, // Dummy headroom
                false,
                STREAM_SETUP_TIMEOUT);

        // Reconfigure flow control using the new frequency and headroom
        return _mgmt_portal->config_remote_stream(*_ctrl_xport,
            dst_epid,
            src_epid,
            lossy_xport,
            _get_buff_params_ratio(buff_params, fc_freq_ratio),
            _get_buff_params_ratio(buff_params, fc_headroom_ratio),
            reset,
            STREAM_SETUP_TIMEOUT);
    }

    chdr_tx_data_xport::uptr create_host_to_device_data_stream(const sep_addr_t dst_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        _ensure_ep_is_reachable(dst_addr);

        // Generate a new destination (device) EPID instance
        const sep_id_t dst_epid =
            _epid_alloc->allocate_epid(dst_addr, *_mgmt_portal, *_ctrl_xport);

        if (!_mgmt_portal->get_endpoint_info(dst_epid).has_data) {
            throw uhd::rfnoc_error("Downstream endpoint does not support data traffic");
        }

        // Create a new source (host) endpoint and EPID
        {
            std::lock_guard<std::mutex> ep_lock(_data_ep_lock);
            if (_data_src_ep_map.count(dst_addr) == 0) {
                _data_src_ep_map[dst_addr] = SEP_INST_DATA_BASE + _data_ep_inst++;
            }
        }
        const sep_addr_t sw_epid_addr(_my_device_id, _data_src_ep_map[dst_addr]);
        const sep_id_t src_epid = _epid_alloc->allocate_epid(sw_epid_addr);
        _allocated_epids.insert(src_epid);

        return _mb_iface.make_tx_data_transport(*_mgmt_portal,
            {sw_epid_addr, dst_addr},
            {src_epid, dst_epid},
            pyld_buff_fmt,
            mdata_buff_fmt,
            xport_args,
            streamer_id);
    }

    chdr_rx_data_xport::uptr create_device_to_host_data_stream(sep_addr_t src_addr,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const device_addr_t& xport_args,
        const std::string& streamer_id) override
    {
        _ensure_ep_is_reachable(src_addr);

        // Generate a new source (device) EPID instance
        const sep_id_t src_epid =
            _epid_alloc->allocate_epid(src_addr, *_mgmt_portal, *_ctrl_xport);

        if (!_mgmt_portal->get_endpoint_info(src_epid).has_data) {
            throw uhd::rfnoc_error("Downstream endpoint does not support data traffic");
        }

        // Create a new destination (host) endpoint and EPID
        {
            std::lock_guard<std::mutex> ep_lock(_data_ep_lock);
            if (_data_dst_ep_map.count(src_addr) == 0) {
                _data_dst_ep_map[src_addr] = SEP_INST_DATA_BASE + _data_ep_inst++;
            }
        }
        const sep_addr_t sw_epid_addr(_my_device_id, _data_dst_ep_map[src_addr]);
        const sep_id_t dst_epid = _epid_alloc->allocate_epid(sw_epid_addr);
        _allocated_epids.insert(dst_epid);

        return _mb_iface.make_rx_data_transport(*_mgmt_portal,
            {src_addr, sw_epid_addr},
            {src_epid, dst_epid},
            pyld_buff_fmt,
            mdata_buff_fmt,
            xport_args,
            streamer_id);
    }

private:
    void _ensure_ep_is_reachable(const sep_addr_t& ep_addr_)
    {
        for (const auto& ep_addr : _mgmt_portal->get_reachable_endpoints()) {
            if (ep_addr == ep_addr_)
                return;
        }
        throw uhd::routing_error("Specified endpoint is not reachable");
    }

    stream_buff_params_t _get_buff_params_ratio(
        const stream_buff_params_t& buff_params, const double ratio)
    {
        return {static_cast<uint64_t>(std::ceil(double(buff_params.bytes) * ratio)),
            static_cast<uint32_t>(std::ceil(double(buff_params.packets) * ratio))};
    }

    // A reference to the packet factory
    const chdr::chdr_packet_factory& _pkt_factory;
    // The device address of this software endpoint
    const device_id_t _my_device_id;
    // The host adapter ID associated with this software endpoint
    adapter_id_t _my_adapter_id;

    // Motherboard interface
    mb_iface& _mb_iface;
    // A pointer to the EPID allocator
    epid_allocator::sptr _epid_alloc;
    // A set of all allocated EPIDs
    std::set<sep_id_t> _allocated_epids;
    // The software EPID for all management and control traffic
    sep_id_t _my_mgmt_ctrl_epid;
    // Transports
    chdr_ctrl_xport::sptr _ctrl_xport;
    // Management portal for control endpoints
    mgmt_portal::uptr _mgmt_portal;
    // The CHDR control endpoint
    chdr_ctrl_endpoint::uptr _ctrl_ep;
    // A map of all client zero instances indexed by the destination
    std::map<sep_id_t, client_zero::sptr> _client_zero_map;

    // Data endpoint mutex
    std::mutex _data_ep_lock;
    // Data endpoint instance
    sep_inst_t _data_ep_inst;

    // Maps to cache local data endpoints for re-use.  Assumes each connection
    // between the host and a stream endpoint on a given device is unique for
    // each direction.  Re-using enpdoints is needed because the routing table
    // in the FPGA is limited in how many entries can be made.
    std::map<sep_addr_t, sep_inst_t> _data_src_ep_map;
    std::map<sep_addr_t, sep_inst_t> _data_dst_ep_map;
};

link_stream_manager::uptr link_stream_manager::make(
    const chdr::chdr_packet_factory& pkt_factory,
    mb_iface& mb_if,
    const epid_allocator::sptr& epid_alloc,
    device_id_t device_id)
{
    return std::make_unique<link_stream_manager_impl>(
        pkt_factory, mb_if, epid_alloc, device_id);
}
