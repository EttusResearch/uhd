//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_NEON_AD9361_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_NEON_AD9361_IFACE_HPP

#include <uhd/types/direction.hpp>
#include <uhd/types/filters.hpp>
#include <uhd/types/sensors.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;

ad9361_ctrl::sptr make_rpc(rpc_client::sptr rpcc);
std::string get_which_ad9361_chain(const direction_t dir, const size_t chan);

#endif /* INCLUDED_LIBUHD_RFNOC_NEON_AD9361_IFACE_HPP */
