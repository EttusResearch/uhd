//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <b300_experts.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>

namespace b300 = uhd::usrp::b300;

// Convenience constant matching the ADRV9032 LO range used in production code.
static const uhd::freq_range_t ADRV9032_LO_RANGE(450e6, 7.125e9, 1e3);

// ============================================================================
// rflo_frequency_allowed tests
// ============================================================================

// For a 125 MHz master clock rate the DAC sampling rate is 1500 MHz.
// Blackout frequencies for each m: i * (1500 / (2*m)) MHz
//   m=2  -> multiples of 375 MHz  (e.g. 750, 1125 MHz)
//   m=4  -> multiples of 187.5 MHz (e.g. 562.5, 750 MHz)
//   m=10 -> multiples of  75 MHz  (e.g. 750, 900 MHz)

BOOST_AUTO_TEST_CASE(rflo_frequency_allowed_non_blackout_125MHz)
{
    // 802 MHz is more than 1 MHz from every blackout frequency at 125 MHz MCR.
    BOOST_CHECK(b300::rflo_frequency_allowed(802e6, 125e6));
    // 490 MHz is more than 1 MHz from every blackout frequency at 125 MHz MCR.
    BOOST_CHECK(b300::rflo_frequency_allowed(490e6, 125e6));
}

BOOST_AUTO_TEST_CASE(rflo_frequency_allowed_blackout_125MHz)
{
    // 750 MHz = 2 * (1500/4) is a blackout frequency for m=2 at 125 MHz MCR.
    BOOST_CHECK(!b300::rflo_frequency_allowed(750e6, 125e6));
    // 562.5 MHz = 3 * (1500/8) is a blackout frequency for m=4 at 125 MHz MCR.
    BOOST_CHECK(!b300::rflo_frequency_allowed(562.5e6, 125e6));
    // 1125 MHz = 3 * (1500/4) is a blackout frequency for m=2 at 125 MHz MCR.
    BOOST_CHECK(!b300::rflo_frequency_allowed(1125e6, 125e6));
}

// Frequencies within 1 MHz (exclusive) of a blackout are also forbidden.
BOOST_AUTO_TEST_CASE(rflo_frequency_allowed_within_1MHz_of_blackout_125MHz)
{
    // 500 kHz above the 750 MHz blackout.
    BOOST_CHECK(!b300::rflo_frequency_allowed(750.5e6, 125e6));
    // 500 kHz below the 750 MHz blackout.
    BOOST_CHECK(!b300::rflo_frequency_allowed(749.5e6, 125e6));
}

// A frequency exactly 1 MHz from the nearest blackout is at the boundary and
// should be considered allowed (the comparison is strictly less-than).
BOOST_AUTO_TEST_CASE(rflo_frequency_allowed_at_1MHz_boundary_125MHz)
{
    // Exactly 1 MHz above the 750 MHz blackout.
    BOOST_CHECK(b300::rflo_frequency_allowed(751e6, 125e6));
    // Exactly 1 MHz below the 750 MHz blackout.
    BOOST_CHECK(b300::rflo_frequency_allowed(749e6, 125e6));
}

// Verify the logic is correct for a different master clock rate (122.88 MHz).
// dac_sampling_rate = 122.88 * 12 = 1474.56 MHz.
// For m=2: blackout at i * (1474.56/4) = i * 368.64 MHz; i=2 gives 737.28 MHz.
BOOST_AUTO_TEST_CASE(rflo_frequency_allowed_122p88MHz_mcr)
{
    // 737.28 MHz is exactly on a blackout for m=2 at 122.88 MHz MCR.
    BOOST_CHECK(!b300::rflo_frequency_allowed(737.28e6, 122.88e6));
    // 739.28 MHz is 2 MHz above the blackout, well outside the 1 MHz window.
    BOOST_CHECK(b300::rflo_frequency_allowed(739.28e6, 122.88e6));
}

// ============================================================================
// avoid_blackout_frequencies tests
// ============================================================================

// A frequency that is already allowed must be returned unchanged.
BOOST_AUTO_TEST_CASE(avoid_blackout_frequencies_already_allowed)
{
    BOOST_CHECK_EQUAL(
        b300::avoid_blackout_frequencies(802e6, 125e6, ADRV9032_LO_RANGE), 802e6);
}

// Starting on a blackout, the result must be allowed and within MAX_BLACKOUT_AVOIDANCE.
BOOST_AUTO_TEST_CASE(avoid_blackout_frequencies_on_blackout)
{
    const double result =
        b300::avoid_blackout_frequencies(750e6, 125e6, ADRV9032_LO_RANGE);

    BOOST_CHECK(b300::rflo_frequency_allowed(result, 125e6));
    BOOST_CHECK_LE(std::abs(result - 750e6), b300::MAX_BLACKOUT_AVOIDANCE);
}

// The search traverses upward first.  Starting at 750 MHz with a step size of 1 kHz,
// the first allowed frequency reached is 751 MHz (exactly 1 MHz above, at which point
// the distance from 750 MHz is no longer strictly less than 1 MHz for any m-value).
BOOST_AUTO_TEST_CASE(avoid_blackout_frequencies_finds_upward_result)
{
    BOOST_CHECK_EQUAL(
        b300::avoid_blackout_frequencies(750e6, 125e6, ADRV9032_LO_RANGE), 751e6);
}

// Starting at the 562.5 MHz blackout (m=4), the first allowed frequency going up
// is 563.5 MHz (1 MHz away).
BOOST_AUTO_TEST_CASE(avoid_blackout_frequencies_562p5_MHz)
{
    BOOST_CHECK_EQUAL(
        b300::avoid_blackout_frequencies(562.5e6, 125e6, ADRV9032_LO_RANGE), 563.5e6);
}

// Verify a second MCR: 737.28 MHz blackout for 122.88 MHz MCR.
// First allowed going up: 738.28 MHz (exactly 1 MHz above).
BOOST_AUTO_TEST_CASE(avoid_blackout_frequencies_122p88MHz_mcr)
{
    const double result =
        b300::avoid_blackout_frequencies(737.28e6, 122.88e6, ADRV9032_LO_RANGE);

    BOOST_CHECK(b300::rflo_frequency_allowed(result, 122.88e6));
    BOOST_CHECK_LE(std::abs(result - 737.28e6), b300::MAX_BLACKOUT_AVOIDANCE);
}
