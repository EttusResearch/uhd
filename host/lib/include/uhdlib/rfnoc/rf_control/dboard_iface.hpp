//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhdlib/rfnoc/rf_control/gain_profile_iface.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <memory>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Interface that daughterboards expose to the motherboard radio_control
 *
 * This interface contains everything required for a daughterboard to implement
 * all the methods required for radio_control. For the most part, this class
 * just includes accessors to objects which implement the required functionality.
 * This class also directly implements core_iface for the remainder.
 */
class dboard_iface : virtual public core_iface
{
public:
    using sptr = std::shared_ptr<dboard_iface>;

    virtual ~dboard_iface() = default;

    virtual gain_profile_iface::sptr get_tx_gain_profile_api() = 0;
    virtual gain_profile_iface::sptr get_rx_gain_profile_api() = 0;

    virtual size_t get_chan_from_dboard_fe(
        const std::string&, uhd::direction_t) const                                  = 0;
    virtual std::string get_dboard_fe_from_chan(size_t chan, uhd::direction_t) const = 0;

    virtual std::vector<uhd::usrp::pwr_cal_mgr::sptr>& get_pwr_mgr(
        uhd::direction_t trx) = 0;

    virtual uhd::eeprom_map_t get_db_eeprom() = 0;

    //! See radio_control::set_command_time()
    virtual void set_command_time(uhd::time_spec_t time, const size_t chan) = 0;
};

}}} // namespace uhd::rfnoc::rf_control
