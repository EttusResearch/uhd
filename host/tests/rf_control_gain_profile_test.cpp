//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/ranges.hpp>
#include <uhdlib/rfnoc/rf_control/gain_profile_iface.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::rfnoc::rf_control;

void test_profile_invariant(gain_profile_iface& gain_profile)
{
    BOOST_CHECK(!gain_profile.get_gain_profile_names(0).empty());
    for (auto& profile : gain_profile.get_gain_profile_names(0)) {
        gain_profile.set_gain_profile(profile, 0);
        BOOST_CHECK(gain_profile.get_gain_profile(0) == profile);
    }
}

BOOST_AUTO_TEST_CASE(test_default_profile_get_set)
{
    default_gain_profile gain_profile;
    test_profile_invariant(gain_profile);
}

BOOST_AUTO_TEST_CASE(test_enumerated_profile_get_set)
{
    enumerated_gain_profile gain_profile(
        {"ProfileA", "ProfileB", "ProfileC", "ProfileD"}, "ProfileD", 1);
    test_profile_invariant(gain_profile);
}
