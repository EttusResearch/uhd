//
// Copyright 2011-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <string>

namespace uhd{ namespace usrp{

    void apply_tx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const fs_path db_path,
        const fs_path tx_fe_corr_path,
        const double tx_lo_freq //actual lo freq
    );

    void apply_tx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const std::string &slot, //name of dboard slot
        const double tx_lo_freq //actual lo freq
    );
    void apply_rx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const std::string &slot, //name of dboard slot
        const double rx_lo_freq //actual lo freq
    );

    void apply_rx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const fs_path db_path,
        const fs_path rx_fe_corr_path,
        const double rx_lo_freq //actual lo freq
    );
}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP */
