//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/error.h>
#include <uhd/types/device_addr.hpp>
#include <chrono>
#include <cstdint>

namespace uhd { namespace usrp {

const uint32_t X300_FW_RESET_REG  = 0x100058;
const uint32_t X300_FW_RESET_DATA = 1;

const std::chrono::milliseconds X300_FW_RESET_SLEEP_TIME =
    std::chrono::milliseconds(5000UL);

UHD_API uhd_error x300_fw_reset(const uhd::device_addr_t& dev_addr,
    std::chrono::milliseconds sleep_time = X300_FW_RESET_SLEEP_TIME);

}} // namespace uhd::usrp
