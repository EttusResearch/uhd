//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_MAGNESIUM_GAIN_TABLE_HPP
#define INCLUDED_LIBUHD_MAGNESIUM_GAIN_TABLE_HPP

#include <uhd/types/direction.hpp>

namespace magnesium {

/*! Store all gain-table related settings for the N310 (Magnesium) daughterboard
 *
 * For every requested gain value, one of these will be returned. Works for TX
 * and RX paths.
 */
struct gain_tuple_t
{
    //! Attenuation value of the DSA in dB
    double dsa_att;
    //! Attenuation value of Mykonos (AD9371) in dB
    double ad9371_att;
    //! If true, bypass LNA or PA section
    bool bypass;
};


/*! Given a gain index, return a tuple of gain-related settings
 */
gain_tuple_t get_gain_tuple(
    const double gain_index,
    const double freq,
    const uhd::direction_t dir
);

} /* namespace magnesium */

#endif
