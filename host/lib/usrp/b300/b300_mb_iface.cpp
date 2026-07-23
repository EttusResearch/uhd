//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_mb_iface.hpp"
#include "b300_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>

using namespace uhd::rfnoc;

namespace uhd { namespace usrp { namespace b300 {

static uhd::usrp::io_service_args_t get_default_io_srv_args()
{
    uhd::usrp::io_service_args_t args;
    args.recv_offload = false;
    args.send_offload = false;
    return args;
}

b300_mb_iface::b300_mb_iface(b300_pcie_manager::sptr pcie_mgr,
    const uhd::compat_num32 fpga_compat,
    const double master_clock_rate,
    const chdr_w_t chdr_width,
    const uint16_t rfnoc_proto_ver,
    const device_id_t device_id)
    : _pcie_mgr(pcie_mgr)
    , _fpga_compat(fpga_compat)
    , _remote_dev_id(device_id)
    , _bus_clk(std::make_shared<uhd::rfnoc::clock_iface>(
          "bus_clk", B300_BUS_CLOCK_RATE, false))
    , _radio_clk(std::make_shared<uhd::rfnoc::clock_iface>(
          "radio_clk", master_clock_rate, false))
    , _rfnoc_proto_ver(rfnoc_proto_ver)
    , _chdr_width(chdr_width)
    , _pkt_factory(chdr_width, ENDIANNESS_LITTLE)
{
    _pcie_mgr->poke32(FPGA_DEVICE_ID_REG, device_id); // Set the remote device ID
    _bus_clk->set_running(true);
    _radio_clk->set_running(true);
}

b300_mb_iface::~b300_mb_iface() = default;

uint16_t b300_mb_iface::get_proto_ver()
{
    return _rfnoc_proto_ver;
}

chdr_w_t b300_mb_iface::get_chdr_w()
{
    return _chdr_width;
}

uhd::endianness_t b300_mb_iface::get_endianness(const device_id_t /*local_device_id*/)
{
    return _pkt_factory.get_endianness();
}

device_id_t b300_mb_iface::get_remote_device_id()
{
    return _remote_dev_id;
}

std::vector<device_id_t> b300_mb_iface::get_local_device_ids()
{
    return _pcie_mgr->get_local_device_ids();
}

uhd::transport::adapter_id_t b300_mb_iface::get_adapter_id(
    const device_id_t local_device_id)
{
    return _adapter_map[local_device_id];
}

void b300_mb_iface::reset_network()
{
    // noop
}

clock_iface::sptr b300_mb_iface::get_clock_iface(
    const std::string& clock_name, const uint8_t /*unused*/)
{
    if (clock_name == "radio_clk") {
        return _radio_clk;
    }
    if (clock_name == "bus_clk") {
        return _bus_clk;
    }
    throw uhd::key_error("[B300] Invalid timebase clock name: " + clock_name);
}

chdr_ctrl_xport::sptr b300_mb_iface::make_ctrl_transport(
    device_id_t local_device_id, const sep_id_t& local_epid)
{
    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    // std::ignore - send/recv buffer sizes, lossy xport?, packet fc?, enable fc?
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        std::ignore,
        std::ignore,
        std::ignore) = _pcie_mgr->get_links(uhd::transport::link_type_t::CTRL,
        local_device_id,
        local_epid,
        sep_id_t(),
        uhd::device_addr_t(),
        _chdr_width);

    // Associate local device ID with the adapter.
    _adapter_map[local_device_id] = send_link->get_send_adapter_id();

    auto io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, uhd::transport::link_type_t::CTRL);
    auto io_srv_mgr = this->get_io_srv_mgr();
    auto xport      = chdr_ctrl_xport::make(io_srv,
        send_link,
        recv_link,
        _pkt_factory,
        local_epid,
        send_link->get_num_send_frames(),
        recv_link->get_num_recv_frames(),
        send_link->get_send_frame_size(),
        [io_srv_mgr, send_link, recv_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        });
    return xport;
}

chdr_rx_data_xport::uptr b300_mb_iface::make_rx_data_transport(
    mgmt::mgmt_portal& mgmt_portal,
    const sep_addr_pair_t& addrs,
    const sep_id_pair_t& epids,
    const sw_buff_t pyld_buff_fmt,
    const sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    const sep_addr_t local_sep_addr = addrs.second;
    const sep_id_t remote_epid      = epids.first;
    const sep_id_t local_epid       = epids.second;

    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    size_t recv_buff_size;
    bool enable_fc;
    // std::ignore - send buffer sizes, lossy xport?, packet fc?
    std::tie(send_link,
        std::ignore,
        recv_link,
        recv_buff_size,
        std::ignore,
        std::ignore,
        enable_fc) = _pcie_mgr->get_links(uhd::transport::link_type_t::RX_DATA,
        local_sep_addr.first,
        local_epid,
        remote_epid,
        xport_args,
        _chdr_width);

    // Associate local device ID with the adapter.
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    const double ratio = 1.0 / 32;
    stream_buff_params_t fc_freq;
    if (enable_fc) {
        fc_freq = {static_cast<uint64_t>(std::ceil(double(recv_buff_size) * ratio)),
            MAX_FC_FREQ_PKTS};
    } else {
        fc_freq = {0, 0};
    }

    uhd::transport::io_service::sptr cfg_io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, uhd::transport::link_type_t::CTRL);
    auto io_srv_mgr = get_io_srv_mgr();
    uhd::transport::disconnect_callback_t disconnect_cb =
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        };

    chdr_rx_data_xport::fc_params_t fc_params =
        chdr_rx_data_xport::configure_sep(cfg_io_srv,
            recv_link,
            send_link,
            _pkt_factory,
            mgmt_portal,
            epids,
            pyld_buff_fmt,
            mdata_buff_fmt,
            {recv_buff_size, MAX_FC_CAPACITY_PKTS},
            fc_freq,
            {0, 0}, // flow control headroom
            false, // b300 always uses a lossless xport
            xport_args,
            disconnect_cb);

    cfg_io_srv.reset();

    // Connect the links to an I/O service
    uhd::transport::io_service::sptr io_srv = get_io_srv_mgr()->connect_links(recv_link,
        send_link,
        uhd::transport::link_type_t::RX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    return std::make_unique<chdr_rx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        recv_link->get_num_recv_frames(),
        fc_params,
        xport_args,
        disconnect_cb);
}

chdr_tx_data_xport::uptr b300_mb_iface::make_tx_data_transport(
    mgmt::mgmt_portal& mgmt_portal,
    const sep_addr_pair_t& addrs,
    const sep_id_pair_t& epids,
    const sw_buff_t pyld_buff_fmt,
    const sw_buff_t mdata_buff_fmt,
    const uhd::device_addr_t& xport_args,
    const std::string& streamer_id)
{
    const sep_addr_t local_sep_addr = addrs.first;
    const sep_id_t remote_epid      = epids.second;
    const sep_id_t local_epid       = epids.first;

    uhd::transport::send_link_if::sptr send_link;
    uhd::transport::recv_link_if::sptr recv_link;
    // std::ignore - send/recv buffer sizes, lossy xport?, packet fc?, enable fc?
    std::tie(send_link,
        std::ignore,
        recv_link,
        std::ignore,
        std::ignore,
        std::ignore,
        std::ignore) = _pcie_mgr->get_links(uhd::transport::link_type_t::TX_DATA,
        local_sep_addr.first,
        local_epid,
        remote_epid,
        xport_args,
        _chdr_width);

    // Associate local device ID with the adapter.
    _adapter_map[local_sep_addr.first] = send_link->get_send_adapter_id();

    const double fc_freq_ratio     = 1.0 / 8;
    const double fc_headroom_ratio = 0;

    uhd::transport::io_service::sptr cfg_io_srv = get_io_srv_mgr()->connect_links(
        recv_link, send_link, uhd::transport::link_type_t::CTRL);
    auto io_srv_mgr = get_io_srv_mgr();
    uhd::transport::disconnect_callback_t disconnect_cb =
        [io_srv_mgr, recv_link, send_link]() {
            io_srv_mgr->disconnect_links(recv_link, send_link);
        };

    const auto [fc_params, strc_pyld] = chdr_tx_data_xport::configure_sep(cfg_io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        mgmt_portal,
        epids,
        pyld_buff_fmt,
        mdata_buff_fmt,
        xport_args,
        fc_freq_ratio,
        fc_headroom_ratio,
        disconnect_cb);

    cfg_io_srv.reset();

    // Connect the links to an I/O service
    uhd::transport::io_service::sptr io_srv = get_io_srv_mgr()->connect_links(recv_link,
        send_link,
        uhd::transport::link_type_t::TX_DATA,
        get_default_io_srv_args(),
        xport_args,
        streamer_id);

    return std::make_unique<chdr_tx_data_xport>(io_srv,
        recv_link,
        send_link,
        _pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        fc_params,
        strc_pyld,
        disconnect_cb);
}

std::map<std::string, uhd::device_addr_t> b300_mb_iface::get_chdr_xport_adapters()
{
    // No Remote streaming available for a PCIe-only device.
    return {};
}

int b300_mb_iface::add_remote_chdr_route(
    const std::string&, const sep_id_t, const uhd::device_addr_t&)
{
    // No Remote streaming available for a PCIe-only device.
    return 0;
}

}}} // namespace uhd::usrp::b300
