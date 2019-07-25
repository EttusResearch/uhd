//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_mboard_type.hpp"
#include "x300_fw_common.h"
#include "x300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/lexical_cast.hpp>

using namespace uhd::usrp::x300;

x300_mboard_t uhd::usrp::x300::map_pid_to_mb_type(const uint32_t pid)
{
    switch (pid) {
        case X300_USRP_PCIE_SSID_ADC_33:
        case X300_USRP_PCIE_SSID_ADC_18:
            return USRP_X300_MB;
        case X310_USRP_PCIE_SSID_ADC_33:
        case X310_2940R_40MHz_PCIE_SSID_ADC_33:
        case X310_2940R_120MHz_PCIE_SSID_ADC_33:
        case X310_2942R_40MHz_PCIE_SSID_ADC_33:
        case X310_2942R_120MHz_PCIE_SSID_ADC_33:
        case X310_2943R_40MHz_PCIE_SSID_ADC_33:
        case X310_2943R_120MHz_PCIE_SSID_ADC_33:
        case X310_2944R_40MHz_PCIE_SSID_ADC_33:
        case X310_2950R_40MHz_PCIE_SSID_ADC_33:
        case X310_2950R_120MHz_PCIE_SSID_ADC_33:
        case X310_2952R_40MHz_PCIE_SSID_ADC_33:
        case X310_2952R_120MHz_PCIE_SSID_ADC_33:
        case X310_2953R_40MHz_PCIE_SSID_ADC_33:
        case X310_2953R_120MHz_PCIE_SSID_ADC_33:
        case X310_2954R_40MHz_PCIE_SSID_ADC_33:
        case X310_USRP_PCIE_SSID_ADC_18:
        case X310_2940R_40MHz_PCIE_SSID_ADC_18:
        case X310_2940R_120MHz_PCIE_SSID_ADC_18:
        case X310_2942R_40MHz_PCIE_SSID_ADC_18:
        case X310_2942R_120MHz_PCIE_SSID_ADC_18:
        case X310_2943R_40MHz_PCIE_SSID_ADC_18:
        case X310_2943R_120MHz_PCIE_SSID_ADC_18:
        case X310_2944R_40MHz_PCIE_SSID_ADC_18:
        case X310_2945R_PCIE_SSID_ADC_18:
        case X310_2950R_40MHz_PCIE_SSID_ADC_18:
        case X310_2950R_120MHz_PCIE_SSID_ADC_18:
        case X310_2952R_40MHz_PCIE_SSID_ADC_18:
        case X310_2952R_120MHz_PCIE_SSID_ADC_18:
        case X310_2953R_40MHz_PCIE_SSID_ADC_18:
        case X310_2953R_120MHz_PCIE_SSID_ADC_18:
        case X310_2954R_40MHz_PCIE_SSID_ADC_18:
        case X310_2955R_PCIE_SSID_ADC_18:
            return USRP_X310_MB;
        case X310_2974_PCIE_SSID_ADC_18:
            return USRP_X310_MB_NI_2974;
        default:
            return UNKNOWN;
    }
    UHD_THROW_INVALID_CODE_PATH();
}

std::string uhd::usrp::x300::map_mb_type_to_product_name(
    const x300_mboard_t mb_type, const std::string& default_name)
{
    switch (mb_type) {
        case USRP_X300_MB:
            return "X300";
        case USRP_X310_MB:
            return "X310";
        case USRP_X310_MB_NI_2974:
            return "NI-2974";
        default:
            return default_name;
    }
}

size_t uhd::usrp::x300::get_and_check_hw_rev(const mboard_eeprom_t& mb_eeprom)
{
    size_t hw_rev;
    if (mb_eeprom.has_key("revision") and not mb_eeprom["revision"].empty()) {
        try {
            hw_rev = boost::lexical_cast<size_t>(mb_eeprom["revision"]);
        } catch (...) {
            throw uhd::runtime_error(
                "Revision in EEPROM is invalid! Please reprogram your EEPROM.");
        }
    } else {
        throw uhd::runtime_error("No revision detected. MB EEPROM must be reprogrammed!");
    }

    size_t hw_rev_compat = 0;
    if (hw_rev >= 7) { // Revision compat was added with revision 7
        if (mb_eeprom.has_key("revision_compat")
            and not mb_eeprom["revision_compat"].empty()) {
            try {
                hw_rev_compat = boost::lexical_cast<size_t>(mb_eeprom["revision_compat"]);
            } catch (...) {
                throw uhd::runtime_error("Revision compat in EEPROM is invalid! Please "
                                         "reprogram your EEPROM.");
            }
        } else {
            throw uhd::runtime_error(
                "No revision compat detected. MB EEPROM must be reprogrammed!");
        }
    } else {
        // For older HW just assume that revision_compat = revision
        hw_rev_compat = hw_rev;
    }

    if (hw_rev_compat > X300_REVISION_COMPAT) {
        throw uhd::runtime_error(
            std::string("Hardware is too new for this software. Please upgrade to "
                        "a driver that supports hardware revision ")
            + std::to_string(hw_rev));
    } else if (hw_rev < X300_REVISION_MIN) { // Compare min against the revision (and
                                             // not compat) to give us more leeway for
                                             // partial support for a compat
        throw uhd::runtime_error(
            std::string("Software is too new for this hardware. Please downgrade "
                        "to a driver that supports hardware revision ")
            + std::to_string(hw_rev));
    }

    return hw_rev;
}

x300_mboard_t uhd::usrp::x300::get_mb_type_from_eeprom(
    const uhd::usrp::mboard_eeprom_t& mb_eeprom)
{
    if (not mb_eeprom["product"].empty()) {
        uint16_t product_num = 0;
        try {
            product_num = boost::lexical_cast<uint16_t>(mb_eeprom["product"]);
        } catch (const boost::bad_lexical_cast&) {
            product_num = 0;
        }

        return map_pid_to_mb_type(product_num);
    }

    UHD_LOG_WARNING("X300", "Unable to read product ID from EEPROM!");
    return UNKNOWN;
}

std::string uhd::usrp::x300::get_fpga_option(uhd::wb_iface::sptr zpu_ctrl)
{
    // Possible options:
    // 1G  = {0:1G, 1:1G} w/ DRAM, HG  = {0:1G, 1:10G} w/ DRAM, XG  = {0:10G, 1:10G} w/
    // DRAM HA  = {0:1G, 1:Aurora} w/ DRAM, XA  = {0:10G, 1:Aurora} w/ DRAM
    uint32_t sfp0_type = zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_SFP0_TYPE));
    uint32_t sfp1_type = zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_SFP1_TYPE));

    if (sfp0_type == RB_SFP_1G_ETH and sfp1_type == RB_SFP_1G_ETH) {
        return "1G";
    } else if (sfp0_type == RB_SFP_1G_ETH and sfp1_type == RB_SFP_10G_ETH) {
        return "HG";
    } else if (sfp0_type == RB_SFP_10G_ETH and sfp1_type == RB_SFP_10G_ETH) {
        return "XG";
    } else if (sfp0_type == RB_SFP_1G_ETH and sfp1_type == RB_SFP_AURORA) {
        return "HA";
    } else if (sfp0_type == RB_SFP_10G_ETH and sfp1_type == RB_SFP_AURORA) {
        return "XA";
    }
    return "HG"; // Default
}
