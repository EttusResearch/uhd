//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/dboards/magnesium_manager.hpp>
#include <mpm/ad937x/ad937x_spi_iface.hpp>

using namespace mpm::dboards;
using namespace mpm::chips;

magnesium_manager::magnesium_manager(
    const std::string &mykonos_spidev,
    const size_t deserializer_lane_xbar
) : _spi_mutex(std::make_shared<std::mutex>())
  , _spi_lock(mpm::types::lockable::make(_spi_mutex))
  , _mykonos_ctrl(ad937x_ctrl::make(
        _spi_mutex,
        deserializer_lane_xbar,
        make_ad937x_iface(mykonos_spidev),
        mpm::ad937x::gpio::gain_pins_t()
    ))
{
}

