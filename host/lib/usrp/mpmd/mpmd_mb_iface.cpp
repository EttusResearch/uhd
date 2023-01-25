//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_mb_iface.hpp"
#include "mpmd_link_if_mgr.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/device_id.hpp>
#include <uhdlib/utils/compat_check.hpp>

using namespace uhd::rfnoc;
using namespace uhd::mpmd;

namespace {

constexpr uhd::compat_num<size_t, size_t> REMOTE_XPORT_CAP_MIN{4, 3};

}

static uhd::usrp::io_service_args_t get_default_io_srv_args()
{
    // TODO: Need better defaults, taking into account the link type and ensuring
    // that the number of frames is appropriate
    uhd::usrp::io_service_args_t args;
    args.recv_offload = false;
    args.send_offload = false;
    return args;
}

mpmd_mboard_impl::mpmd_mb_iface::mpmd_mb_iface(
    const uhd::device_addr_t& mb_args, uhd::rpc_client::sptr rpc)
    : _mb_args(mb_args), _rpc(rpc), _link_if_mgr(xport::mpmd_link_if_mgr::make(mb_args))
{
    _remote_device_id = allocate_device_id();
    UHD_LOG_TRACE("MPMD::MB_IFACE", "Assigning device_id " << _remote_device_id);
    _rpc->notify_with_token("set_device_id", _remote_device_id);

    // Check for remote streaming capabilities
    auto compat_num = rpc->request<std::vector<size_t>>("get_mpm_compat_num");
    _has_remote_xport_capability =
        uhd::compat_num<size_t, size_t>(compat_num[0], compat_num[1])
        >= REMOTE_XPORT_CAP_MIN;
}

/******************************************************************************
 * mpmd_mb_iface API
 *****************************************************************************/
void mpmd_mboard_impl::mpmd_mb_iface::init()
{
    UHD_LOG_TRACE("MPMD::MB_IFACE", "Requesting clock ifaces...");
    auto clock_ifaces = _rpc->request_with_token<clock_iface_list_t>("get_clocks");
    for (auto& clock : clock_ifaces) {
        auto iface = std::make_shared<uhd::rfnoc::clock_iface>(
            clock.at("name"), std::stod(clock.at("freq")), clock.count("mutable"));
        _clock_ifaces[clock.at("name")] = iface;
        iface->set_running(true);
        UHD_LOG_DEBUG("MPMD::MB_IFACE",
            "Adding clock iface `"
                << clock.at("name") << "`, frequency: " << (iface->get_freq() / 1e6)
                << " MHz, mutable: " << (iface->is_mutable() ? "Yes" : "No"));
    }
    UHD_LOG_TRACE("MPMD::MB_IFACE", "Requesting CHDR link types...");
    auto chdr_link_types =
        _rpc->request_with_token<std::vector<std::string>>("get_chdr_link_types");
    UHD_LOG_TRACE(
        "MPMD::MB_IFACE", "Found " << chdr_link_types.size() << " link type(s)");
    for (const auto& type : chdr_link_types) {
        UHD_LOG_TRACE("MPMD::MB_IFACE", "Trying link type `" << type << "'");
        const auto xport_info =
            _rpc->request_with_token<xport::mpmd_link_if_mgr::xport_info_list_t>(
                "get_chdr_link_options", type);
        // User may have specified: addr=192.168.10.2, second_addr=
        // MPM may have said: "my addresses are 192.168.10.2 and 192.168.20.2"
        if (_link_if_mgr->connect(type, xport_info, get_chdr_w())) {
            UHD_LOG_TRACE("MPMD::MB_IFACE", "Link type " << type << " successful.");
        }
    }

    if (_link_if_mgr->get_num_links() == 0) {
        UHD_LOG_ERROR("MPMD::MB_IFACE", "No CHDR connection available!");
        throw uhd::runtime_error("No CHDR connection available!");
    }

    for (size_t link_idx = 0; link_idx < _link_if_mgr->get_num_links(); link_idx++) {
        _local_device_id_map.emplace(allocate_device_id(), link_idx);
    }
}

/******************************************************************************
 * mb_iface API
 *****************************************************************************/
uint16_t mpmd_mboard_impl::mpmd_mb_iface::get_proto_ver()
{
    return _rpc->request<uint16_t>("get_proto_ver");
}

uhd::rfnoc::chdr_w_t mpmd_mboard_impl::mpmd_mb_iface::get_chdr_w()
{
    const auto chdr_w_bits = _rpc->request<size_t>("get_chdr_width");
    switch (chdr_w_bits) {
        case 512:
            return CHDR_W_512;
        case 256:
            return CHDR_W_256;
        case 128:
            return CHDR_W_128;
        case 64:
            return CHDR_W_64;
    }
    throw uhd::runtime_error(std::string("Device reporting invalid CHDR width: ")
                             + std::to_string(chdr_w_bits));
}

uhd::endianness_t mpmd_mboard_impl::mpmd_mb_iface::get_endianness(
    const uhd::rfnoc::device_id_t local_device_id)
{
    uhd::rfnoc::device_id_t lookup_id = local_device_id;
    if (lookup_id == NULL_DEVICE_ID) {
        for (auto& ids : _local_device_id_map) {
            lookup_id = ids.first;
            break;
        }
    }
    const size_t link_idx = _local_device_id_map.at(lookup_id);
    auto& pkt_factory     = _link_if_mgr->get_packet_factory(link_idx);
    return pkt_factory.get_endianness();
}

uhd::rfnoc::device_id_t mpmd_mboard_impl::mpmd_mb_iface::get_remote_device_id()
{
    return _remote_device_id;
}

std::vector<device_id_t> mpmd_mboard_impl::mpmd_mb_iface::get_local_device_ids()
{
    std::vector<device_id_t> device_ids;
    for (auto& local_dev_id_pair : _local_device_id_map) {
        device_ids.push_back(local_dev_id_pair.first);
    }
    return device_ids;
}

uhd::transport::adapter_id_t mpmd_mboard_impl::mpmd_mb_iface::get_adapter_id(
    const uhd::rfnoc::device_id_t local_device_id)
{
    return _adapter_map.at(local_device_id);
}

void mpmd_mboard_impl::mpmd_mb_iface::reset_network()
{
    // FIXME
}

uhd::rfnoc::clock_iface::sptr mpmd_mboard_impl::mpmd_mb_iface::get_clock_iface(
    const std::string& clock_name)
{
    if (_clock_ifaces.count(clock_name)) {
        return _clock_ifaces.at(clock_name);
    } else {
        UHD_LOG_ERROR("MPMD::MB_IFACE", "Invalid timebase clock name: " + clock_name);
        throw uhd::key_error(
            "[MPMD_MB::IFACE] Invalid timebase clock name: " + clock_name);
    }
}

uhd::rfnoc::chdr_ctrl_xport::sptr mpmd_mboard_impl::mpmd_mb_iface::make_ctrl_transport(
    uhd::rfnoc::device_id_t local_device_id, const uhd::rfnoc::sep_id_t& local_epid)
{
    if (!_local_device_id_map.count(local_device_id)) {
        throw uhd::key_error(std::string("[MPMD::MB_IFACE] Cannot create control "
                                         "transport: Unknown local device ID ")
                             + std::to_string(local_device_id));
    }
    const size_t link_idx = _local_device_id_map.at(local_device_id);
    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        std::ignore,
        std::ignore,
        std::ignore) = _link_if_mgr->get_link(link_idx,
        uhd::transport::link_type_t::CTRL,
        uhd::device_addr_t());

    /* Associate local device ID with the adapter */
    _adapter_map[local_device_id] = send_link->get_send_adapter_id();

    auto io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, transport::link_type_t::CTRL);

    auto pkt_factory = _link_if_mgr->get_packet_factory(link_idx);
    auto io_srv_mgr = this->get_io_srv_mgr();
    auto xport       = uhd::rfnoc::chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        pkt_factory,
        local_epid,
        send_link->get_num_send_frames(),
        recv_link->get_num_recv_frames(),
        [io_srv_mgr, send_link, recv_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });
    return xport;
}

uhd::rfnoc::chdr_rx_data_xport::uptr
mpmd_mboard_impl::mpmd_mb_iface::make_rx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.second;

    if (!_local_device_id_map.count(local_sep_addr.first)) {
        throw uhd::key_error(std::string("[MPMD::MB_IFACE] Cannot create RX data "
                                         "transport: Unknown local device ID ")
                             + std::to_string(local_sep_addr.first));
    }
    const size_t link_idx = _local_device_id_map.at(local_sep_addr.first);

    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    bool lossy_xport, packet_fc, enable_fc;
    size_t recv_buff_size;
    std::tie(send_link,
        std::ignore,
        recv_link,
        recv_buff_size,
        lossy_xport,
        packet_fc,
        enable_fc) = _link_if_mgr->get_link(link_idx,
        uhd::transport::link_type_t::RX_DATA,
        xport_args);

    /* Associate local device ID with the adapter */
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    const uhd::rfnoc::stream_buff_params_t recv_capacity = {recv_buff_size,
        packet_fc ? static_cast<uint32_t>(recv_link->get_num_recv_frames())
                  : uhd::rfnoc::MAX_FC_CAPACITY_PKTS};

    const double ratio = 1.0 / 32;

    uhd::rfnoc::stream_buff_params_t fc_freq;
    if (enable_fc) {
        // Configure flow control frequency to use either bytes only or packets only
        if (packet_fc) {
            fc_freq = {uhd::rfnoc::MAX_FC_FREQ_BYTES,
                static_cast<uint32_t>(
                    std::ceil(recv_link->get_num_recv_frames() * ratio))};
        } else {
            fc_freq = {static_cast<uint64_t>(std::ceil(double(recv_buff_size) * ratio)),
                uhd::rfnoc::MAX_FC_FREQ_PKTS};
        }
    } else {
        fc_freq = {0, 0};
    }

    stream_buff_params_t fc_headroom = {0, 0};

    auto cfg_io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, transport::link_type_t::CTRL);

    // Create the data transport
    auto pkt_factory = _link_if_mgr->get_packet_factory(link_idx);
    auto io_srv_mgr = this->get_io_srv_mgr();
    auto fc_params   = chdr_rx_data_xport::configure_sep(cfg_io_srv,
        recv_link,
        send_link,
        pkt_factory,
        mgmt_portal,
        epids,
        pyld_buff_fmt,
        mdata_buff_fmt,
        recv_capacity,
        fc_freq,
        fc_headroom,
        lossy_xport,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    cfg_io_srv.reset();

    // Connect the links to an I/O service
    auto io_srv = get_io_srv_mgr()->connect_links(recv_link,
        send_link,
        transport::link_type_t::RX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    auto rx_xport = std::make_unique<chdr_rx_data_xport>(io_srv,
        recv_link,
        send_link,
        pkt_factory,
        epids,
        recv_link->get_num_recv_frames(),
        fc_params,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    return rx_xport;
}

uhd::rfnoc::chdr_tx_data_xport::uptr
mpmd_mboard_impl::mpmd_mb_iface::make_tx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.first;

    if (!_local_device_id_map.count(local_sep_addr.first)) {
        throw uhd::key_error(std::string("[MPMD::MB_IFACE] Cannot create TX data "
                                         "transport: Unknown local device ID ")
                             + std::to_string(local_sep_addr.first));
    }
    const size_t link_idx = _local_device_id_map.at(local_sep_addr.first);

    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    bool lossy_xport;
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        lossy_xport,
        std::ignore,
        std::ignore) = _link_if_mgr->get_link(link_idx,
        uhd::transport::link_type_t::TX_DATA,
        xport_args);

    /* Associate local device ID with the adapter */
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    // TODO: configure this based on the transport type
    const double fc_freq_ratio     = 1.0 / 8;
    const double fc_headroom_ratio = 0;

    auto cfg_io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, transport::link_type_t::CTRL);

    auto pkt_factory         = _link_if_mgr->get_packet_factory(link_idx);
    auto io_srv_mgr = this->get_io_srv_mgr();
    const auto buff_capacity = chdr_tx_data_xport::configure_sep(cfg_io_srv,
        recv_link,
        send_link,
        pkt_factory,
        mgmt_portal,
        epids,
        pyld_buff_fmt,
        mdata_buff_fmt,
        fc_freq_ratio,
        fc_headroom_ratio,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    cfg_io_srv.reset();

    // Connect the links to an I/O service
    auto io_srv = get_io_srv_mgr()->connect_links(recv_link,
        send_link,
        transport::link_type_t::TX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    // Create the data transport
    auto tx_xport = std::make_unique<chdr_tx_data_xport>(io_srv,
        recv_link,
        send_link,
        pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        buff_capacity,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    return tx_xport;
}

std::map<std::string, uhd::device_addr_t>
mpmd_mboard_impl::mpmd_mb_iface::get_chdr_xport_adapters()
{
    if (!_has_remote_xport_capability) {
        return {};
    }

    // Note: If MPM is capable, but the FPGA is not, then MPM will return an
    // empty list here.
    const auto xport_adapters = _rpc->request_with_token<
        std::map<std::string, std::map<std::string, std::string>>>(
        "get_chdr_xport_adapters");

    // Convert to UHD data type and return
    std::map<std::string, uhd::device_addr_t> return_val;
    for (auto& adapter_info : xport_adapters) {
        return_val.insert({adapter_info.first, uhd::device_addr_t(adapter_info.second)});
    }
    return return_val;
}

int mpmd_mboard_impl::mpmd_mb_iface::add_remote_chdr_route(const std::string& adapter_id,
    const uhd::rfnoc::sep_id_t epid,
    const uhd::device_addr_t& route_args)
{
    if (!_has_remote_xport_capability) {
        throw uhd::not_implemented_error("This version of MPM does not have the "
                                         "capability to add remote CHDR routes!");
    }
    const auto adapters = get_chdr_xport_adapters();
    if (!adapters.count(adapter_id)) {
        throw uhd::value_error(
            "Invalid adapter for creating a remote route: " + adapter_id);
    }
    std::map<std::string, std::string> route_args_map;
    for (const auto& key : route_args.keys()) {
        route_args_map[key] = route_args.get(key);
    }
    return _rpc->request_with_token<int>(
        "add_remote_chdr_route", adapter_id, epid, route_args_map);
}
