//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "obx/obx_cpld_ctrl.hpp"
#include "obx/obx_gpio_ctrl.hpp"
#include <uhd/experts/expert_factory.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhdlib/usrp/common/max287x.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

/***********************************************************************
 * OBX Constants
 **********************************************************************/
static const freq_range_t obx_freq_range(10e6, 8.4e9);
static const freq_range_t obx_bw_range(160e6, 160e6);
static const gain_range_t obx_gain_range(0, 31.5, double(0.5));
static const std::vector<std::string> obx_tx_antennas{"TX/RX", "CAL"};
static const std::vector<std::string> obx_rx_antennas{"TX/RX", "RX2", "CAL"};
static const std::vector<std::string> obx_power_modes{"performance", "powersave"};
static const std::vector<std::string> obx_xcvr_modes{"FDX", "TDD", "TX", "RX"};
static const std::vector<std::string> obx_temp_comp_modes{"enabled", "disabled"};
enum power_mode_t { PERFORMANCE, POWERSAVE };
enum xcvr_mode_t { FDX, TDD, TX, RX };
static const dboard_id_t OBX_TX_ID(0x3500);
static const dboard_id_t OBX_RX_ID(0x3501);
static const std::vector<dboard_id_t> obx_ids{OBX_TX_ID, OBX_RX_ID};

class obx_xcvr : public xcvr_dboard_base
{
public:
    obx_xcvr(ctor_args_t args);
    ~obx_xcvr();

private:
    void _initialize_ref_clocks();
    void _initialize_los();
    void _initialize_property_tree();
    void _initialize_experts();
    void _add_register_abstractions();
    void _calibrate_vco_maps(uhd::direction_t dir);

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    dboard_iface::sptr _db_iface;
    obx_cpld_ctrl::sptr _cpld;
    obx_gpio_ctrl::sptr _gpio;
    std::shared_ptr<max287x_iface> _txlo1;
    std::shared_ptr<max287x_iface> _txlo2;
    std::shared_ptr<max287x_iface> _rxlo1;
    std::shared_ptr<max287x_iface> _rxlo2;
    tmp468_iface::sptr _temp_sensor;
    double _tx_target_pfd_freq;
    double _rx_target_pfd_freq;
    double _tx_freq;
    double _rx_freq;
    uhd::experts::expert_container::sptr _expert_container;
};

// This function is to intended to mirror UBX for determining db clock rate
static UHD_INLINE double get_max_pfd_freq(dboard_id_t)
{
    return 50e6;
}

}}}} // namespace uhd::usrp::dboard::obx
