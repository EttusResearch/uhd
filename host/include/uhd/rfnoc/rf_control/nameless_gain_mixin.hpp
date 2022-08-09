//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhd/types/direction.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Partially implements core_iface for the gain functions which take no name parameter
 */
class UHD_API nameless_gain_mixin : virtual public core_iface
{
public:
    using name_selector =
        std::function<std::string(const uhd::direction_t trx, const size_t chan)>;

    /*! Sets the name selector for the mixin. The closure receives the direction
     * of the call and the channel, and returns the gain to use for the call. The
     * name selector may simply return a string, or may do a more complex algorithm.
     */
    nameless_gain_mixin(name_selector name_selector);

    virtual ~nameless_gain_mixin() = default;

    double set_tx_gain(const double gain, const size_t chan) override;
    double get_tx_gain(const size_t chan) override;

    double set_rx_gain(const double gain, const size_t chan) override;
    double get_rx_gain(const size_t chan) override;

    // Getting the gain ranges is a bit different - these always use the empty name
    gain_range_t get_tx_gain_range(const size_t chan) const override;
    gain_range_t get_rx_gain_range(const size_t chan) const override;

private:
    name_selector _name_selector;

    using core_iface::get_tx_gain;
    using core_iface::get_tx_gain_range;
    using core_iface::set_tx_gain;

    using core_iface::get_rx_gain;
    using core_iface::get_rx_gain_range;
    using core_iface::set_rx_gain;
};

}}} // namespace uhd::rfnoc::rf_control
