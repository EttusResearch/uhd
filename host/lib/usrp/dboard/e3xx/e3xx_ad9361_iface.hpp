//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E3XX_AD9361_IFACE_HPP
#    define INCLUDED_LIBUHD_RFNOC_E3XX_AD9361_IFACE_HPP

#    include <uhd/types/direction.hpp>
#    include <uhd/types/filters.hpp>
#    include <uhd/types/sensors.hpp>
#    include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#    include <uhdlib/utils/rpc.hpp>
#    include <memory>
#    include <string>
#    include <vector>

using namespace uhd;
using namespace uhd::usrp;

static constexpr size_t E3XX_TUNE_TIMEOUT = 60000;
static constexpr size_t E3XX_RATE_TIMEOUT = 60000;
ad9361_ctrl::sptr make_rpc(rpc_client::sptr rpcc);
std::string get_which_ad9361_chain(
    const direction_t dir, const size_t chan, const bool fe_swap = false);

#endif /* INCLUDED_LIBUHD_RFNOC_E3XX_AD9361_IFACE_HPP */
// vim: sw=4 et:
