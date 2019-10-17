//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_MB_IFACE_HPP
#define INCLUDED_MPMD_MB_IFACE_HPP

#include "mpmd_impl.hpp"
#include "mpmd_link_if_mgr.hpp"
#include <uhdlib/rfnoc/mb_iface.hpp>
#include <uhdlib/usrp/common/io_service_mgr.hpp>
#include <unordered_map>
#include <map>

namespace uhd { namespace mpmd {

class mpmd_mboard_impl::mpmd_mb_iface : public uhd::rfnoc::mb_iface
{
public:
    using uptr               = std::unique_ptr<mpmd_mb_iface>;
    using clock_iface_list_t = std::vector<std::map<std::string, std::string>>;
    mpmd_mb_iface(const uhd::device_addr_t& mb_args, uhd::rpc_client::sptr rpc);
    ~mpmd_mb_iface() = default;

    /*** mpmd_mb_iface API calls *****************************************/
    //! Initialize transports
    void init();

    /*** mb_iface API calls **********************************************/
    uint16_t get_proto_ver();
    uhd::rfnoc::chdr_w_t get_chdr_w();
    uhd::endianness_t get_endianness(const uhd::rfnoc::device_id_t local_device_id);
    uhd::rfnoc::device_id_t get_remote_device_id();
    std::vector<uhd::rfnoc::device_id_t> get_local_device_ids();
    uhd::transport::adapter_id_t get_adapter_id(
        const uhd::rfnoc::device_id_t local_device_id);
    void reset_network();
    uhd::rfnoc::clock_iface::sptr get_clock_iface(const std::string& clock_name);
    uhd::rfnoc::chdr_ctrl_xport::sptr make_ctrl_transport(
        uhd::rfnoc::device_id_t local_device_id, const uhd::rfnoc::sep_id_t& local_epid);
    uhd::rfnoc::chdr_rx_data_xport::uptr make_rx_data_transport(
        uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const uhd::device_addr_t& xport_args,
        const std::string& streamer_id);
    uhd::rfnoc::chdr_tx_data_xport::uptr make_tx_data_transport(
        uhd::rfnoc::mgmt::mgmt_portal& mgmt_portal,
        const uhd::rfnoc::sep_addr_pair_t& addrs,
        const uhd::rfnoc::sep_id_pair_t& epids,
        const uhd::rfnoc::sw_buff_t pyld_buff_fmt,
        const uhd::rfnoc::sw_buff_t mdata_buff_fmt,
        const uhd::device_addr_t& xport_args,
        const std::string& streamer_id);

private:
    uhd::device_addr_t _mb_args;
    uhd::rpc_client::sptr _rpc;
    xport::mpmd_link_if_mgr::uptr _link_if_mgr;
    uhd::rfnoc::device_id_t _remote_device_id;
    std::map<uhd::rfnoc::device_id_t, size_t> _local_device_id_map;
    std::unordered_map<uhd::rfnoc::device_id_t, uhd::transport::adapter_id_t>
        _adapter_map;
    std::map<std::string, uhd::rfnoc::clock_iface::sptr> _clock_ifaces;
    uhd::usrp::io_service_mgr::sptr _io_srv_mgr;
};

}} /* namespace uhd::mpmd */

#endif /* INCLUDED_MPMD_MB_IFACE_HPP */
