//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <mpm/dboards/magnesium_manager.hpp>
#include <mpm/chips/lmk04828_spi_iface.hpp>
#include <mpm/ad937x/ad937x_spi_iface.hpp>

using namespace mpm::dboards;
using namespace mpm::chips;

magnesium_manager::magnesium_manager(
    const std::string &lmk_spidev,
    const std::string &mykonos_spidev
) : _spi_mutex(std::make_shared<std::mutex>())
  , _spi_lock(mpm::types::lockable::make(_spi_mutex))
  , _clock_ctrl(mpm::chips::make_lmk04828_iface(lmk_spidev))
  , _mykonos_ctrl(ad937x_ctrl::make(
        _spi_mutex,
        make_ad937x_iface(mykonos_spidev),
        mpm::ad937x::gpio::gain_pins_t()
    ))
{

}

