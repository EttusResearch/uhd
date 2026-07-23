//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_clock_ctrl.hpp"
#include "b300_mb_controller.hpp"
#include "b300_mb_iface.hpp"
#include <uhdlib/rfnoc/rfnoc_device.hpp>

namespace uhd { namespace usrp { namespace b300 {

static const uint8_t B300_FPGA_COMPAT_NUM_MAJOR = 2;
static const uint8_t B300_FPGA_COMPAT_NUM_MINOR = 0;
static const uint16_t B300_REVISION_COMPAT      = 3;
static const uint16_t B300_REVISION_MIN         = 1;
// If the module has a blank eeprom, we will log a warning and fallback to revision C. We
// don't want to just error in that case as it then doesn't allow the eeprom programming
// utility to run and fix the issue.
static const uint16_t B300_FALLBACK_REVISION = 3;

enum b300_product_t { B310 };

uhd::device_addrs_t b300_find(const uhd::device_addr_t& hint);

class b300_impl : public uhd::rfnoc::detail::rfnoc_device
{
public:
    // structors
    b300_impl(const uhd::device_addr_t& device_addr);
    void setup_mb(const size_t mb_idx, const uhd::device_addr_t& dev_args);
    ~b300_impl(void) override;

    uhd::rfnoc::mb_iface& get_mb_iface(const size_t mb_idx) override;

private:
    uhd::compat_num32 check_fpga_compat(
        const uhd::fs_path& mb_path, b300_pcie_manager::sptr pcie_mgr);
    uint16_t get_and_check_hw_rev(const mboard_eeprom_t& mb_eeprom);
    // Stores the args with which the device was originally initialized
    uhd::device_addr_t _device_args;
    // Stores a list of motherboard interfaces
    std::unordered_map<size_t, b300_mb_iface> _mb_ifaces;
    // Stores a list of motherboard controllers
    std::unordered_map<size_t, uhd::rfnoc::b300_mb_controller::sptr> _mb_controllers;
};

}}} // namespace uhd::usrp::b300
