//
// Copyright 2011-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/iq_dc_cal_coeffs.hpp>
#include <string>

namespace uhd { namespace usrp {

/*! Apply TX DC offset or IQ imbalance corrections (RFNoC version)
 *
 * \param sub_tree Property tree object
 * \param db_serial Daughterboard serial
 * \param tx_fe_corr_path This is the path relative to \p sub_tree where the
 *                        coefficients are stored. The path should end in
 *                        `iq_balance/value` or `dc_offset/value` and be a
 *                        complex number.
 * \param tx_lo_freq The current LO frequency. Used to look up coefficients in the cal
 *                   data set.
 */
void apply_tx_fe_corrections(property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& db_serial,
    const fs_path tx_fe_corr_path,
    const double tx_lo_freq // actual lo freq
);

/*! Apply TX DC offset or IQ imbalance corrections (Gen-2 USRP version)
 *
 * \param sub_tree Property tree object. It's the motherboard subtree, i.e.,
 *                 what comes after /mboards/X.
 * \param slot Daughterboard slot ("A" or "B"), used to auto-detect the paths.
 * \param tx_lo_freq The current LO frequency. Used to look up coefficients in the cal
 *                   data set.
 */
void apply_tx_fe_corrections(property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& slot, // name of dboard slot
    const double tx_lo_freq // actual lo freq
);

/*! Apply RX DC offset or IQ imbalance corrections (RFNoC version)
 *
 * \param sub_tree Property tree object
 * \param db_serial Daughterboard serial
 * \param tx_fe_corr_path This is the path relative to \p sub_tree where the
 *                        coefficients are stored. The path should end in
 *                        `iq_balance/value` or `dc_offset/value` and be a
 *                        complex number.
 * \param tx_lo_freq The current LO frequency. Used to look up coefficients in the cal
 *                   data set.
 */
void apply_rx_fe_corrections(property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& db_serial,
    const fs_path rx_fe_corr_path,
    const double rx_lo_freq // actual lo freq
);

/*! Apply RX DC offset or IQ imbalance corrections (Gen-2 USRP version)
 *
 * \param sub_tree Property tree object. It's the motherboard subtree, i.e.,
 *                 what comes after /mboards/X.
 * \param slot Daughterboard slot ("A" or "B"), used to auto-detect the paths.
 * \param rx_lo_freq The current LO frequency. Used to look up coefficients in the cal
 *                   data set.
 */
void apply_rx_fe_corrections(property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& slot, // name of dboard slot
    const double rx_lo_freq // actual lo freq
);

/*! Get wideband TX IQ imbalance and DC offset corrections
 *
 * \param db_serial Daughterboard serial
 * \param sample_rate The current sample rate. Used to construct the cal key.
 * \param chan The channel index.
 * \param tx_freq The current TX frequency.
 */
uhd::iq_dc_cal_coeffs_t get_wideband_tx_iq_dc_corrections(const std::string& db_serial,
    const double sample_rate,
    const size_t chan,
    const double tx_freq);

/*! Get wideband RX IQ imbalance and DC offset corrections
 *
 * \param db_serial Daughterboard serial
 * \param sample_rate The current sample rate. Used to construct the cal key.
 * \param chan The channel index.
 * \param rx_freq The current RX frequency.
 */
uhd::iq_dc_cal_coeffs_t get_wideband_rx_iq_dc_corrections(const std::string& db_serial,
    const double sample_rate,
    const size_t chan,
    const double rx_freq);

}} // namespace uhd::usrp
