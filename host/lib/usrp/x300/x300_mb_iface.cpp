//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include <uhdlib/rfnoc/device_id.hpp>

using namespace uhd::rfnoc;
using uhd::transport::link_type_t;


x300_impl::x300_mb_iface::x300_mb_iface(uhd::usrp::x300::conn_manager::sptr conn_mgr,
    const double radio_clk_freq,
    const uhd::rfnoc::device_id_t remote_dev_id)
    : _remote_dev_id(remote_dev_id)
    , _bus_clk(std::make_shared<uhd::rfnoc::clock_iface>(
          "bus_clk", uhd::usrp::x300::BUS_CLOCK_RATE, false))
    , _radio_clk(
          std::make_shared<uhd::rfnoc::clock_iface>("radio_clk", radio_clk_freq, false))
    , _conn_mgr(conn_mgr)
{
    UHD_ASSERT_THROW(_conn_mgr);
    _bus_clk->set_running(true);
    _radio_clk->set_running(true);
}

x300_impl::x300_mb_iface::~x300_mb_iface() = default;

uint16_t x300_impl::x300_mb_iface::get_proto_ver()
{
    // TODO: Get from from a hardware register
    return 0x0100;
}

uhd::rfnoc::chdr_w_t x300_impl::x300_mb_iface::get_chdr_w()
{
    // TODO: Get from from a hardware register
    return uhd::rfnoc::CHDR_W_64;
}

uhd::endianness_t x300_impl::x300_mb_iface::get_endianness(
    const uhd::rfnoc::device_id_t /*local_device_id*/)
{
    // FIXME
    return uhd::ENDIANNESS_BIG;
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
    uhd::transport::io_service::sptr io_srv;
    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    std::tie(io_srv, send_link, std::ignore, recv_link, std::ignore, std::ignore) =
        _conn_mgr->get_links(link_type_t::CTRL,
            local_device_id,
            local_epid,
            uhd::rfnoc::sep_id_t(),
            uhd::device_addr_t());

    /* Associate local device ID with the adapter */
    _adapter_map[local_device_id] = send_link->get_send_adapter_id();

    auto xport = uhd::rfnoc::chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        _pkt_factory,
        local_epid,
        send_link->get_num_send_frames(),
        recv_link->get_num_recv_frames());
    return xport;
}

uhd::rfnoc::chdr_rx_data_xport::uptr x300_impl::x300_mb_iface::make_rx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args)
{
    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.second;
    const uhd::rfnoc::sep_id_t remote_epid      = epids.first;
    const uhd::rfnoc::sep_id_t local_epid       = epids.second;

    uhd::transport::io_service::sptr io_srv;
    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    size_t recv_buff_size;
    bool lossy_xport;
    std::tie(io_srv, send_link, std::ignore, recv_link, recv_buff_size, lossy_xport) =
        _conn_mgr->get_links(link_type_t::RX_DATA,
            local_sep_addr.first,
            local_epid,
            remote_epid,
            xport_args);

    /* Associate local device ID with the adapter */
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    // TODO: configure this based on the transport type
    const uhd::rfnoc::stream_buff_params_t recv_capacity = {
        recv_buff_size, uhd::rfnoc::MAX_FC_CAPACITY_PKTS};

    const double ratio = 1.0 / 32;

    // Configure flow control frequency to use bytes only for UDP
    uhd::rfnoc::stream_buff_params_t fc_freq = {
        static_cast<uint64_t>(std::ceil(double(recv_buff_size) * ratio)),
        uhd::rfnoc::MAX_FC_FREQ_PKTS};

    uhd::rfnoc::stream_buff_params_t fc_headroom = {0, 0};

    // Create the data transport
    auto fc_params = uhd::rfnoc::chdr_rx_data_xport::configure_sep(io_srv,
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
        lossy_xport);

    auto rx_xport = std::make_unique<uhd::rfnoc::chdr_rx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        recv_link->get_num_recv_frames(),
        fc_params);

    return rx_xport;
}

uhd::rfnoc::chdr_tx_data_xport::uptr x300_impl::x300_mb_iface::make_tx_data_transport(
    uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
    const uhd::rfnoc::sep_addr_pair_t& addrs,
    const uhd::rfnoc::sep_id_pair_t& epids,
    const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
    const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args)
{
    const uhd::rfnoc::sep_addr_t local_sep_addr = addrs.first;
    const uhd::rfnoc::sep_id_t remote_epid      = epids.second;
    const uhd::rfnoc::sep_id_t local_epid       = epids.first;

    uhd::transport::io_service::sptr io_srv;
    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    bool lossy_xport;
    std::tie(io_srv, send_link, std::ignore, recv_link, std::ignore, lossy_xport) =
        _conn_mgr->get_links(link_type_t::TX_DATA,
            local_sep_addr.first,
            local_epid,
            remote_epid,
            xport_args);

    /* Associate local device ID with the adapter */
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    // TODO: configure this based on the transport type
    const double fc_freq_ratio     = 1.0 / 8;
    const double fc_headroom_ratio = 0;

    const auto buff_capacity = chdr_tx_data_xport::configure_sep(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        mgmt_portal,
        epids,
        pyld_buff_fmt,
        mdata_buff_fmt,
        fc_freq_ratio,
        fc_headroom_ratio);

    // Create the data transport
    auto tx_xport = std::make_unique<chdr_tx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        buff_capacity);

    return tx_xport;
}
