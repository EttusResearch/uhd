//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/rf_control/gain_profile_iface.hpp>
#include <stddef.h>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

const std::string default_gain_profile::DEFAULT_GAIN_PROFILE = "default";

std::vector<std::string> default_gain_profile::get_gain_profile_names(const size_t) const
{
    return {DEFAULT_GAIN_PROFILE};
}

void default_gain_profile::set_gain_profile(const std::string& profile, const size_t chan)
{
    if (profile != DEFAULT_GAIN_PROFILE) {
        throw uhd::value_error(
            std::string("set_tx_gain_profile(): Unknown gain profile: `") + profile
            + "'");
    }
    if (_sub) {
        _sub(profile, chan);
    }
}

std::string default_gain_profile::get_gain_profile(const size_t) const
{
    return DEFAULT_GAIN_PROFILE;
}


void default_gain_profile::add_subscriber(subscriber_type&& sub)
{
    _sub = std::move(sub);
}

enumerated_gain_profile::enumerated_gain_profile(
    const std::vector<std::string>& possible_profiles,
    const std::string& default_profile,
    size_t num_channels)
    : _possible_profiles(possible_profiles), _gain_profile(num_channels, default_profile)
{
}

void enumerated_gain_profile::set_gain_profile(
    const std::string& profile, const size_t chan)
{
    if (!uhd::has(_possible_profiles, profile)) {
        const std::string err_msg = ("Invalid gain profile provided: " + profile);
        UHD_LOG_ERROR("gain_profile", err_msg);
        throw uhd::key_error(err_msg);
    }
    _gain_profile.at(chan) = profile;
    if (_sub) {
        _sub(profile, chan);
    }
}

std::string enumerated_gain_profile::get_gain_profile(const size_t chan) const
{
    return _gain_profile.at(chan);
}

std::vector<std::string> enumerated_gain_profile::get_gain_profile_names(
    const size_t) const
{
    return _possible_profiles;
}

void enumerated_gain_profile::add_subscriber(subscriber_type&& sub)
{
    _sub = std::move(sub);
}

}}} // namespace uhd::rfnoc::rf_control
