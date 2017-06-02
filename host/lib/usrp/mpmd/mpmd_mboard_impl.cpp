//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "mpmd_impl.hpp"
#include <chrono>
#include <thread>

namespace {
    const size_t MPMD_RECLAIM_INTERVAL_MS = 1000;

    const std::string MPMD_DEFAULT_SESSION_ID = "UHD";
}

using namespace uhd;

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_mboard_impl::mpmd_mboard_impl(
        const device_addr_t &mb_args,
        const std::string& ip_addr
) : rpc(uhd::rpc_client::make(ip_addr, MPM_RPC_PORT))
{
    UHD_LOGGER_TRACE("MPMD")
        << "Initializing mboard, connecting to IP address: " << ip_addr
        << " mboard args: " << mb_args.to_string()
    ;
    auto device_info_dict = rpc->request<dev_info>("get_device_info");
    for (const auto &info_pair: device_info_dict) {
        device_info[info_pair.first] = info_pair.second;
    }
    UHD_LOGGER_TRACE("MPMD")
        << "MPM reports device info: " << device_info.to_string();

    // Claim logic
    auto rpc_token = rpc->request<std::string>("claim",
        mb_args.get("session_id", MPMD_DEFAULT_SESSION_ID)
    );
    if (rpc_token.empty()) {
        throw uhd::value_error("mpmd device claiming failed!");
    }
    UHD_LOG_TRACE("MPMD", "Received claim token " << rpc_token);
    rpc->set_token(rpc_token);
    _claimer_task = task::make([this] {
        if (not this->claim()) {
            throw uhd::value_error("mpmd device reclaiming loop failed!");
        };
        std::this_thread::sleep_for(
            std::chrono::milliseconds(MPMD_RECLAIM_INTERVAL_MS)
        );
    });

    // Initialize properties
    this->num_xbars = rpc->request<size_t>("get_num_xbars");
    // Local addresses are not yet valid after this!
    this->xbar_local_addrs.resize(this->num_xbars, 0xFF);

    // std::vector<std::string> data_ifaces =
    //     rpc.call<std::vector<std::string>>("get_interfaces", rpc_token);

    // discover path to device and tell MPM our MAC address seen at the data
    // interfaces
    // move this into make_transport
    //for (const auto& iface : data_ifaces) {
        //std::vector<std::string> addrs = rpc.call<std::vector<std::string>>(
            //"get_interface_addrs", _rpc_token, iface);
        //for (const auto& iface_addr : addrs) {
            //if (rpc_client(iface_addr, MPM_RPC_PORT)
                    //.call<bool>("probe_interface", _rpc_token)) {
                //data_interfaces.emplace(iface, iface_addr);
                //break;
            //}
        //}
    //}
}

mpmd_mboard_impl::~mpmd_mboard_impl()
{
    /* nop */
}

/*****************************************************************************
 * API
 ****************************************************************************/
uhd::sid_t mpmd_mboard_impl::allocate_sid(const uint16_t port,
                                          const uhd::sid_t address,
                                          const uint32_t xbar_src_addr,
                                          const uint32_t xbar_src_port)
{
    const auto sid = rpc->request_with_token<uint32_t>(
        "allocate_sid",
        port, address.get(), xbar_src_addr, xbar_src_port
    );
    return uhd::sid_t(sid);
}

void mpmd_mboard_impl::set_xbar_local_addr(
        const size_t xbar_index,
        const size_t local_addr
) {
    UHD_ASSERT_THROW(rpc->request_with_token<bool>("set_xbar_local_addr", xbar_index, local_addr));
    UHD_ASSERT_THROW(xbar_index < xbar_local_addrs.size());
    xbar_local_addrs.at(xbar_index) = local_addr;
}

/*****************************************************************************
 * Private methods
 ****************************************************************************/
bool mpmd_mboard_impl::claim()
{
    return rpc->request_with_token<bool>("reclaim");
}

/*****************************************************************************
 * Factory
 ****************************************************************************/
mpmd_mboard_impl::uptr mpmd_mboard_impl::make(
    const uhd::device_addr_t &mb_args,
    const std::string& addr
) {
    mpmd_mboard_impl::uptr mb =
        mpmd_mboard_impl::uptr(new mpmd_mboard_impl(mb_args, addr));
    // implicit move
    return mb;
}

