//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/rf_control/antenna_iface.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <stddef.h>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

enumerated_antenna::enumerated_antenna(uhd::property_tree::sptr tree,
    prop_path prop_path_generator,
    const std::vector<std::string>& possible_antennas,
    const std::unordered_map<std::string, std::string>& compat_map)
    : _tree(tree)
    , _prop_path_generator(prop_path_generator)
    , _possible_antennas(possible_antennas)
    , _compat_map(compat_map)
{
}

std::vector<std::string> enumerated_antenna::get_antennas(const size_t) const
{
    return _possible_antennas;
}

void enumerated_antenna::set_antenna(const std::string& ant, const size_t chan)
{
    if (!_compat_map.count(ant)) {
        assert_has(_possible_antennas, ant, "antenna");
    }

    auto path = _prop_path_generator(chan);

    _tree->access<std::string>(path).set(ant);
}

std::string enumerated_antenna::get_antenna(const size_t chan) const
{
    auto path = _prop_path_generator(chan);
    return _tree->access<std::string>(path).get();
}

std::string antenna_radio_control_mixin::get_tx_antenna(const size_t chan) const
{
    return _tx_antenna->get_antenna(chan);
}

std::vector<std::string> antenna_radio_control_mixin::get_tx_antennas(
    const size_t chan) const
{
    return _tx_antenna->get_antennas(chan);
}

void antenna_radio_control_mixin::set_tx_antenna(
    const std::string& ant, const size_t chan)
{
    _tx_antenna->set_antenna(ant, chan);
}

std::string antenna_radio_control_mixin::get_rx_antenna(const size_t chan) const
{
    return _rx_antenna->get_antenna(chan);
}

std::vector<std::string> antenna_radio_control_mixin::get_rx_antennas(
    const size_t chan) const
{
    return _rx_antenna->get_antennas(chan);
}

void antenna_radio_control_mixin::set_rx_antenna(
    const std::string& ant, const size_t chan)
{
    _rx_antenna->set_antenna(ant, chan);
}

}}} // namespace uhd::rfnoc::rf_control
