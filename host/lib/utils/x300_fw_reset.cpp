//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../usrp/x300/x300_eth_mgr.hpp"
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/x300_fw_reset.hpp>
#include <thread>

namespace uhd { namespace usrp {

uhd_error x300_fw_reset(
    const uhd::device_addr_t& dev_addr, std::chrono::milliseconds sleep_time)
{
    // It is not possible to reset device if IP address is not provided
    if (!dev_addr.has_key("addr")) {
        return UHD_ERROR_INVALID_DEVICE;
    }

    // Reset Scope
    // Create UDP connection
    uhd::transport::udp_simple::sptr udp_simple =
        uhd::transport::udp_simple::make_connected(
            dev_addr["addr"], BOOST_STRINGIZE(X300_FW_COMMS_UDP_PORT));

    // Create X300 control
    uhd::wb_iface::sptr x300_ctrl = x300_make_ctrl_iface_enet(udp_simple, true);

    // Reset FPGA firmware
    x300_ctrl->poke32(X300_FW_RESET_REG, X300_FW_RESET_DATA);

    std::this_thread::sleep_for(sleep_time);
    return UHD_ERROR_NONE;
}

}} // namespace uhd::usrp
