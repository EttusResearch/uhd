//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/rf_control/nameless_gain_mixin.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

nameless_gain_mixin::nameless_gain_mixin(name_selector name_selector)
    : _name_selector(name_selector)
{
}

double nameless_gain_mixin::set_tx_gain(const double gain, const size_t chan)
{
    const auto name = _name_selector(TX_DIRECTION, chan);
    return set_tx_gain(gain, name, chan);
}

double nameless_gain_mixin::get_tx_gain(const size_t chan)
{
    const auto name = _name_selector(TX_DIRECTION, chan);
    return get_tx_gain(name, chan);
}

double nameless_gain_mixin::set_rx_gain(const double gain, const size_t chan)
{
    const auto name = _name_selector(RX_DIRECTION, chan);
    return set_rx_gain(gain, name, chan);
}

double nameless_gain_mixin::get_rx_gain(const size_t chan)
{
    const auto name = _name_selector(RX_DIRECTION, chan);
    return get_rx_gain(name, chan);
}

gain_range_t nameless_gain_mixin::get_tx_gain_range(const size_t chan) const
{
    return get_tx_gain_range("", chan);
}

gain_range_t nameless_gain_mixin::get_rx_gain_range(const size_t chan) const
{
    return get_rx_gain_range("", chan);
}

}}} // namespace uhd::rfnoc::rf_control
