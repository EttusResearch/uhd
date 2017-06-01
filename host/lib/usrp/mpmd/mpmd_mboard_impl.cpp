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
}

using namespace uhd;

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_mboard_impl::mpmd_mboard_impl(const std::string& ip_addr)
    : rpc(uhd::rpc_client::make(ip_addr, MPM_RPC_PORT))
{
    UHD_LOG_TRACE("MPMD", "Initializing mboard, IP address: " << ip_addr);
    auto _dev_info = rpc->call<dev_info>("get_device_info");
    device_info =
        dict<std::string, std::string>(_dev_info.begin(), _dev_info.end());
    // Get initial claim on mboard
    auto rpc_token = rpc->call<std::string>("claim", "UHD - Session 01"); // TODO make this configurable with device_addr, and provide better defaults
    if (rpc_token.empty()) {
        throw uhd::value_error("mpmd device claiming failed!");
    }
    rpc->set_token(rpc_token);
    _claimer_task = task::make([this] {
        if (not this->claim()) {
            throw uhd::value_error("mpmd device reclaiming loop failed!");
        };
        std::this_thread::sleep_for(
            std::chrono::milliseconds(MPMD_RECLAIM_INTERVAL_MS)
        );
    });

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
    const auto sid = rpc->call_with_token<uint32_t>(
        "allocate_sid",
        port, address.get(), xbar_src_addr, xbar_src_port
    );
    return uhd::sid_t(sid);
}


/*****************************************************************************
 * Private methods
 ****************************************************************************/
bool mpmd_mboard_impl::claim()
{
    return rpc->call_with_token<bool>("reclaim");
}

/*****************************************************************************
 * Factory
 ****************************************************************************/
mpmd_mboard_impl::uptr mpmd_mboard_impl::make(const std::string& addr)
{
    mpmd_mboard_impl::uptr mb =
        mpmd_mboard_impl::uptr(new mpmd_mboard_impl(addr));
    // implicit move
    return mb;
}

