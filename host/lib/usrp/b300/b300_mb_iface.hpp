//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_pcie_mgr.hpp"
#include <uhd/utils/compat_check.hpp>
#include <uhdlib/rfnoc/mb_iface.hpp>

namespace uhd { namespace usrp { namespace b300 {

class b300_mb_iface : public uhd::rfnoc::mb_iface
{
public:
    using uptr = std::unique_ptr<b300_mb_iface>;
    b300_mb_iface(b300_pcie_manager::sptr pcie_mgr,
        const uhd::compat_num32 fpga_compat,
        const double master_clock_rate,
        const uhd::rfnoc::chdr_w_t chdr_width,
        const uint16_t rfnoc_proto_ver,
        const uhd::rfnoc::device_id_t device_id);
    ~b300_mb_iface() override;
    uint16_t get_proto_ver() override;
    uhd::rfnoc::chdr_w_t get_chdr_w() override;
    uhd::endianness_t get_endianness(
        const uhd::rfnoc::device_id_t local_device_id) override;
    uhd::rfnoc::device_id_t get_remote_device_id() override;
    std::vector<uhd::rfnoc::device_id_t> get_local_device_ids() override;
    uhd::transport::adapter_id_t get_adapter_id(
        const uhd::rfnoc::device_id_t local_device_id) override;
    void reset_network() override;
    uhd::rfnoc::clock_iface::sptr get_clock_iface(
        const std::string& clock_name, const uint8_t) override;
    uhd::rfnoc::chdr_ctrl_xport::sptr make_ctrl_transport(
        uhd::rfnoc::device_id_t local_device_id,
        const uhd::rfnoc::sep_id_t& local_epid) override;
    uhd::rfnoc::chdr_rx_data_xport::uptr make_rx_data_transport(
        uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const uhd::device_addr_t& xport_args,
        const std::string& streamer_id) override;
    uhd::rfnoc::chdr_tx_data_xport::uptr make_tx_data_transport(
        uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const uhd::device_addr_t& xport_args,
        const std::string& streamer_id) override;
    std::map<std::string, uhd::device_addr_t> get_chdr_xport_adapters() override;
    int add_remote_chdr_route(const std::string& adapter_id,
        const uhd::rfnoc::sep_id_t epid,
        const uhd::device_addr_t& route_args) override;

private:
    b300_pcie_manager::sptr _pcie_mgr;
    uhd::compat_num32 _fpga_compat;
    const uhd::rfnoc::device_id_t _remote_dev_id;
    uhd::rfnoc::clock_iface::sptr _bus_clk;
    uhd::rfnoc::clock_iface::sptr _radio_clk;
    uint16_t _rfnoc_proto_ver;
    uhd::rfnoc::chdr_w_t _chdr_width;
    const uhd::rfnoc::chdr::chdr_packet_factory _pkt_factory;
    std::unordered_map<uhd::rfnoc::device_id_t, uhd::transport::adapter_id_t>
        _adapter_map;
};

}}} // namespace uhd::usrp::b300
