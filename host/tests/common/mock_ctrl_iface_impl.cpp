//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mock_ctrl_iface_impl.hpp"
static const uint64_t TEST_NOC_ID = 0xAAAABBBBCCCCDDDD;

uint64_t mock_ctrl_iface_impl::send_cmd_pkt(const size_t addr,
    const size_t data,
    const bool readback,
    const uint64_t /* timestamp */
)
{
    if (not readback) {
        std::cout << str(boost::format("[MOCK] poke to addr: %016X, data == %016X") % addr
                         % data)
                  << std::endl;
    } else {
        std::cout << str(boost::format("[MOCK] peek64 to addr: %016X") % data)
                  << std::endl;
        switch (data) {
            case uhd::rfnoc::SR_READBACK_REG_ID:
                return TEST_NOC_ID;
            case uhd::rfnoc::SR_READBACK_REG_FIFOSIZE:
                return 0x0000000000010000;
            case uhd::rfnoc::SR_READBACK_REG_USER:
                return 0x0123456789ABCDEF;
            case uhd::rfnoc::SR_READBACK_COMPAT:
                return uint64_t(uhd::rfnoc::NOC_SHELL_COMPAT_MAJOR) << 32
                       | uint64_t(uhd::rfnoc::NOC_SHELL_COMPAT_MINOR);
            default:
                return 0;
        }
    }
    return 0;
}
