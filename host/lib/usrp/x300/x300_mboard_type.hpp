//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_MBOARD_TYPE_HPP
#define INCLUDED_X300_MBOARD_TYPE_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <string>

namespace uhd { namespace usrp { namespace x300 {

enum class xport_path_t { ETH, NIRIO };

enum x300_mboard_t { USRP_X300_MB, USRP_X310_MB, USRP_X310_MB_NI_2974, UNKNOWN };

/*! Return the correct motherboard type for a given product ID
 *
 * Note: In previous versions, we had two different mappings for PCIe and
 * Ethernet in case the PIDs would conflict, but they never did and it was
 * thus consolidated into one.
 */
x300_mboard_t map_pid_to_mb_type(const uint32_t pid);

/*! Map the motherboard type to a product name
 */
std::string map_mb_type_to_product_name(
    const x300_mboard_t mb_type, const std::string& default_name = "");

/*! Read HW revision (and HW compat revision) from mboard and run checks
 *
 * \throws uhd::runtime_error if the MB is not compatible with the SW
 */
size_t get_and_check_hw_rev(const uhd::usrp::mboard_eeprom_t& mb_eeprom);

x300_mboard_t get_mb_type_from_eeprom(const uhd::usrp::mboard_eeprom_t& mb_eeprom);

/*! Look up the FPGA type (XG, HG, etc.) in some ZPU registers
 */
std::string get_fpga_option(uhd::wb_iface::sptr zpu_ctrl);

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_MBOARD_TYPE_HPP */
