//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include <uhdlib/rfnoc/device_id.hpp>

using namespace uhd::rfnoc;
using uhd::transport::link_type_t;

static uhd::usrp::io_service_args_t get_default_io_srv_args()
{
    // TODO: Need better defaults, taking into account the link type and ensuring
    // that the number of frames is appropriate
    uhd::usrp::io_service_args_t args;
    args.recv_offload = false;
    args.send_offload = false;
    return args;
}

x300_impl::x300_mb_iface::x300_mb_iface(uhd::usrp::x300::conn_manager::sptr conn_mgr,
    uhd::wb_iface::sptr zpu_ctrl,
    const double radio_clk_freq,
    const uhd::rfnoc::device_id_t remote_dev_id)
    : _remote_dev_id(remote_dev_id)
    , _bus_clk(std::make_shared<uhd::rfnoc::clock_iface>(
          "bus_clk", uhd::usrp::x300::BUS_CLOCK_RATE, false))
    , _radio_clk(
          std::make_shared<uhd::rfnoc::clock_iface>("radio_clk", radio_clk_freq, false))
    , _conn_mgr(conn_mgr)
    , _zpu_ctrl(zpu_ctrl)
{
    UHD_ASSERT_THROW(_conn_mgr);
    _bus_clk->set_running(true);
    _radio_clk->set_running(true);
}

x300_impl::x300_mb_iface::~x300_mb_iface() = default;

uint16_t x300_impl::x300_mb_iface::get_proto_ver()
{
    const uint32_t rfnoc_info = _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_RFNOC_INFO));
    return ZPU_RB_RFNOC_INFO_PROTOVER(rfnoc_info);
}

uhd::rfnoc::chdr_w_t x300_impl::x300_mb_iface::get_chdr_w()
{
    const uint32_t rfnoc_info = _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_RFNOC_INFO));
    const size_t chdr_width = ZPU_RB_RFNOC_INFO_CHDR_WIDTH(rfnoc_info);
    return bits_to_chdr_w(chdr_width);
}

uhd::endianness_t x300_impl::x300_mb_iface::get_endianness(
    const uhd::rfnoc::device_id_t /*local_device_id*/)
{
    // FIXME
    return uhd::ENDIANNESS_LITTLE;
}

uhd::rfnoc::device_id_t x300_impl::x300_mb_iface::get_remote_device_id()
{
    return _remote_dev_id;
}

std::vector<uhd::rfnoc::device_id_t> x300_impl::x300_mb_iface::get_local_device_ids()
{
    return _conn_mgr->get_local_device_ids();
}

uhd::transport::adapter_id_t x300_impl::x300_mb_iface::get_adapter_id(
    const uhd::rfnoc::device_id_t local_device_id)
{
    return _adapter_map[local_device_id];
}

void x300_impl::x300_mb_iface::reset_network()
{
    // FIXME
}

uhd::rfnoc::clock_iface::sptr x300_impl::x300_mb_iface::get_clock_iface(
    const std::string& clock_name)
{
    if (clock_name == "radio_clk") {
        return _radio_clk;
    }
    if (clock_name == "bus_clk") {
        return _bus_clk;
    }
    UHD_LOG_ERROR("X300", "Invalid timebase clock name: " + clock_name);
    throw uhd::key_error("[X300] Invalid timebase clock name: " + clock_name);
}

uhd::rfnoc::chdr_ctrl_xport::sptr x300_impl::x300_mb_iface::make_ctrl_transport(
    uhd::rfnoc::device_id_t local_device_id, const uhd::rfnoc::sep_id_t& local_epid)
{
    using namespace uhd::transport;

    send_link_if::sptr send_link;
    recv_link_if::sptr recv_link;
    bool lossy_xport;
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        lossy_xport,
        std::ignore,
        std::ignore) = _conn_mgr->get_links(link_type_t::CTRL,
        local_device_id,
        local_epid,
        uhd::rfnoc::sep_id_t(),
        uhd::device_addr_t());

    /* Associate local device ID with the adapter */
    _adapter_map[local_device_id] = send_link->get_send_adapter_id();

    auto io_srv =
        get_io_srv_mgr()->connect_links(recv_link, send_link, link_type_t::CTRL);
    auto io_srv_mgr = this->get_io_srv_mgr();
    auto xport      = chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        _pkt_factory,
        local_epid,
        send_link->get_num_send_frames(),
        recv_link->get_num_recv_frames(),
        [io_srv_mgr, send_link, recv_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });
    return xport;
}

uhd::rfnoc::chdr_rx_data_xport::uptr x300_impl::x300_mb_iface::make_rx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    using namespace uhd::transport;

    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.second;
    const uhd::rfnoc::sep_id_t remote_epid      = epids.first;
    const uhd::rfnoc::sep_id_t local_epid       = epids.second;

    send_link_if::sptr send_link;
    recv_link_if::sptr recv_link;
    size_t recv_buff_size;
    bool lossy_xport, packet_fc, enable_fc;
    std::tie(send_link,
        std::ignore,
        recv_link,
        recv_buff_size,
        lossy_xport,
        packet_fc,
        enable_fc) = _conn_mgr->get_links(link_type_t::RX_DATA,
        local_sep_addr.first,
        local_epid,
        remote_epid,
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

    uhd::rfnoc::stream_buff_params_t fc_headroom = {0, 0};

    auto cfg_io_srv =
        get_io_srv_mgr()->connect_links(recv_link, send_link, link_type_t::CTRL);

    auto io_srv_mgr = this->get_io_srv_mgr();

    auto fc_params = uhd::rfnoc::chdr_rx_data_xport::configure_sep(cfg_io_srv,
        recv_link,
        send_link,
        _pkt_factory,
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
        link_type_t::RX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    // Create the data transport
    auto rx_xport = std::make_unique<uhd::rfnoc::chdr_rx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        recv_link->get_num_recv_frames(),
        fc_params,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    return rx_xport;
}

uhd::rfnoc::chdr_tx_data_xport::uptr x300_impl::x300_mb_iface::make_tx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    using namespace uhd::transport;

    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.first;
    const uhd::rfnoc::sep_id_t remote_epid      = epids.second;
    const uhd::rfnoc::sep_id_t local_epid       = epids.first;

    send_link_if::sptr send_link;
    recv_link_if::sptr recv_link;
    bool lossy_xport;
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        lossy_xport,
        std::ignore,
        std::ignore) = _conn_mgr->get_links(link_type_t::TX_DATA,
        local_sep_addr.first,
        local_epid,
        remote_epid,
        xport_args);

    /* Associate local device ID with the adapter */
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    // TODO: configure this based on the transport type
    const double fc_freq_ratio     = 1.0 / 8;
    const double fc_headroom_ratio = 0;

    auto cfg_io_srv =
        get_io_srv_mgr()->connect_links(recv_link, send_link, link_type_t::CTRL);

    auto io_srv_mgr = this->get_io_srv_mgr();

    const auto buff_capacity = chdr_tx_data_xport::configure_sep(cfg_io_srv,
        recv_link,
        send_link,
        _pkt_factory,
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
        link_type_t::TX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    // Create the data transport
    auto tx_xport = std::make_unique<chdr_tx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        buff_capacity,
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });

    return tx_xport;
}
