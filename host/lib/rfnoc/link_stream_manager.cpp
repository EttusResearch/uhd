//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/transport/muxed_zero_copy_if.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <uhdlib/rfnoc/link_stream_manager.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
#include <boost/format.hpp>

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
        // TODO: This needs to be cleaned up. A muxed_zero_copy_if is excessive here
        _ctrl_xport = _mb_iface.make_ctrl_transport(_my_device_id, _my_mgmt_ctrl_epid);

        // Create management portal using one of the child transports
        _mgmt_portal = mgmt_portal::make(*_ctrl_xport,
            _pkt_factory,
            sep_addr_t(_my_device_id, SEP_INST_MGMT_CTRL),
            _my_mgmt_ctrl_epid);
    }

    virtual ~link_stream_manager_impl()
    {
        for (const auto& epid : _allocated_epids) {
            _epid_alloc->deallocate_epid(epid);
        }
    }

    virtual device_id_t get_self_device_id() const
    {
        return _my_device_id;
    }

    virtual const std::set<sep_addr_t>& get_reachable_endpoints() const
    {
        return _mgmt_portal->get_reachable_endpoints();
    }

    virtual bool can_connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr) const
    {
        return _mgmt_portal->can_remote_route(dst_addr, src_addr);
    }

    virtual sep_id_pair_t connect_host_to_device(sep_addr_t dst_addr)
    {
        _ensure_ep_is_reachable(dst_addr);

        // Allocate EPIDs
        sep_id_t dst_epid = _epid_alloc->allocate_epid(dst_addr);

        // Make sure that the software side of the endpoint is initialized and reachable
        if (_ctrl_ep == nullptr) {
            // Create a control endpoint with that xport
            _ctrl_ep =
                chdr_ctrl_endpoint::make(_ctrl_xport, _pkt_factory, _my_mgmt_ctrl_epid);
        }

        // Setup a route to the EPID
        _mgmt_portal->initialize_endpoint(*_ctrl_xport, dst_addr, dst_epid);
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

    virtual sep_id_pair_t connect_device_to_device(
        sep_addr_t dst_addr, sep_addr_t src_addr)
    {
        _ensure_ep_is_reachable(dst_addr);
        _ensure_ep_is_reachable(src_addr);

        // Allocate EPIDs
        sep_id_t dst_epid = _epid_alloc->allocate_epid(dst_addr);
        sep_id_t src_epid = _epid_alloc->allocate_epid(src_addr);

        // Initialize endpoints
        _mgmt_portal->initialize_endpoint(*_ctrl_xport, dst_addr, dst_epid);
        _mgmt_portal->initialize_endpoint(*_ctrl_xport, src_addr, src_epid);

        _mgmt_portal->setup_remote_route(*_ctrl_xport, dst_epid, src_epid);

        return sep_id_pair_t(src_epid, dst_epid);
    }

    virtual ctrlport_endpoint::sptr get_block_register_iface(sep_id_t dst_epid,
        uint16_t block_index,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk)
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
        uint16_t dst_port = 1 + c0_ctrl->get_num_stream_endpoints() + block_index;
        if (block_index >= c0_ctrl->get_num_blocks()) {
            throw uhd::value_error("Requested block index out of range");
        }

        // Create control endpoint
        return _ctrl_ep->get_ctrlport_ep(dst_epid,
            dst_port,
            (size_t(1) << c0_ctrl->get_block_info(dst_port).ctrl_fifo_size),
            c0_ctrl->get_block_info(dst_port).ctrl_max_async_msgs,
            client_clk,
            timebase_clk);
    }

    virtual client_zero::sptr get_client_zero(sep_id_t dst_epid) const
    {
        if (_client_zero_map.count(dst_epid) == 0) {
            throw uhd::runtime_error(
                "Control for the specified EPID was not initialized");
        }
        return _client_zero_map.at(dst_epid);
    }

    virtual stream_buff_params_t create_device_to_device_data_stream(
        const sep_id_t& dst_epid,
        const sep_id_t& src_epid,
        const bool lossy_xport,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const bool reset = false)
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

        // Compute FC frequency and headroom based on buff parameters
        stream_buff_params_t fc_freq{
            static_cast<uint64_t>(std::ceil(double(buff_params.bytes) * fc_freq_ratio)),
            static_cast<uint32_t>(
                std::ceil(double(buff_params.packets) * fc_freq_ratio))};
        stream_buff_params_t fc_headroom{
            static_cast<uint64_t>(
                std::ceil(double(buff_params.bytes) * fc_headroom_ratio)),
            static_cast<uint32_t>(
                std::ceil(double(buff_params.packets) * fc_headroom_ratio))};

        // Reconfigure flow control using the new frequency and headroom
        return _mgmt_portal->config_remote_stream(*_ctrl_xport,
            dst_epid,
            src_epid,
            lossy_xport,
            fc_freq,
            fc_headroom,
            reset,
            STREAM_SETUP_TIMEOUT);
    }

    virtual chdr_tx_data_xport::uptr create_host_to_device_data_stream(
        const sep_addr_t dst_addr,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const device_addr_t& xport_args)
    {
        // Create a new source endpoint and EPID
        sep_addr_t sw_epid_addr(_my_device_id, SEP_INST_DATA_BASE + (_data_ep_inst++));
        sep_id_t src_epid = _epid_alloc->allocate_epid(sw_epid_addr);
        _allocated_epids.insert(src_epid);
        _ensure_ep_is_reachable(dst_addr);

        // Generate a new destination EPID instance
        sep_id_t dst_epid = _epid_alloc->allocate_epid(dst_addr);

        // Create the data transport that we will return to the client
        chdr_rx_data_xport::uptr xport = _mb_iface.make_rx_data_transport(
            _my_device_id, src_epid, dst_epid, xport_args);

        chdr_ctrl_xport::sptr mgmt_xport =
            _mb_iface.make_ctrl_transport(_my_device_id, src_epid);

        // Create new temporary management portal with the transports used for this stream
        // TODO: This is a bit excessive. Maybe we can pair down the functionality of the
        // portal just for route setup purposes. Whatever we do, we *must* use xport in it
        // though otherwise the transport will not behave correctly.
        mgmt_portal::uptr data_mgmt_portal =
            mgmt_portal::make(*mgmt_xport, _pkt_factory, sw_epid_addr, src_epid);

        // Setup a route to the EPID
        data_mgmt_portal->initialize_endpoint(*mgmt_xport, dst_addr, dst_epid);
        data_mgmt_portal->setup_local_route(*mgmt_xport, dst_epid);
        if (!_mgmt_portal->get_endpoint_info(dst_epid).has_data) {
            throw uhd::rfnoc_error("Downstream endpoint does not support data traffic");
        }

        // TODO: Implement data transport setup logic here


        // Delete the portal when done
        data_mgmt_portal.reset();
        return xport;
    }

    virtual chdr_tx_data_xport::uptr create_device_to_host_data_stream(
        const sep_addr_t src_addr,
        const bool lossy_xport,
        const sw_buff_t pyld_buff_fmt,
        const sw_buff_t mdata_buff_fmt,
        const double fc_freq_ratio,
        const double fc_headroom_ratio,
        const device_addr_t& xport_args)
    {
        // TODO: Implement me
        return chdr_tx_data_xport::uptr();
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

    // A reference to the packet factor
    const chdr::chdr_packet_factory& _pkt_factory;
    // The device address of this software endpoint
    const device_id_t _my_device_id;

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
    // Data endpoint instance
    sep_inst_t _data_ep_inst;
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
