//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/ranges.hpp>

namespace uhd {

/*! Create a total tune range for multi_usrp objects
 *
 * Digital tuning can be used to shift the baseband below / past the tunable
 * limits of the actual RF front-end. The baseband filter, located on the
 * daughterboard, however, limits the useful instantaneous bandwidth. We
 * allow the user to tune to the edge of the filter, where the roll-off
 * begins.  This prevents the user from tuning past the point where less
 * than half of the spectrum would be useful.
 * Also, we make sure that frequencies don't become negative.
 */
static meta_range_t make_overall_tune_range(
    const meta_range_t& fe_range, const meta_range_t& dsp_range, const double bw)
{
    meta_range_t range;
    for (const range_t& sub_range : fe_range) {
        range.push_back(range_t(
            std::max(0.0, sub_range.start() + std::max(dsp_range.start(), -bw / 2)),
            sub_range.stop() + std::min(dsp_range.stop(), bw / 2),
            dsp_range.step()));
    }
    return range;
}


} // namespace uhd
