//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP

#include "../../../utils/rpc.hpp"
#include <uhd/types/direction.hpp>
#include <iostream>
#include <string>

class magnesium_ad9371_iface
{
public:
    using uptr = std::unique_ptr<magnesium_ad9371_iface>;

    magnesium_ad9371_iface(
        uhd::rpc_client::sptr rpcc,
        const size_t slot_idx
    );

    double set_frequency(
        const double freq,
        const size_t chan,
        const uhd::direction_t dir
    );

    double get_frequency(
        const size_t chan,
        const uhd::direction_t dir
    );

    double set_gain(
        const double gain,
        const size_t chan,
        const uhd::direction_t dir
    );

    double get_gain(
        const size_t chan,
        const uhd::direction_t dir
    );

    double set_bandwidth(
        const double bandwidth,
        const size_t chan,
        const uhd::direction_t dir
    );

    double get_bandwidth(
        const size_t chan,
        const uhd::direction_t dir
    );

private:
    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Slot index
    const size_t _slot_idx;

    //! Stores the prefix to RPC calls
    const std::string _rpc_prefix;

    //! Logger prefix
    const std::string _L;



};

#endif /* INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP */
// vim: sw=4 et:
