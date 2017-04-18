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

#include "mpm/dboards/magnesium_manager.hpp"
#include "mpm/spi/spidev_iface.hpp"
#include <boost/make_shared.hpp>

using namespace mpm::dboards;

magnesium_periph_manager::magnesium_periph_manager(
    std::string lmk_spidev, std::string mykonos_spidev
    ): _spi_mutex(std::make_shared<std::mutex>())
{
    _clock_spi = lmk04828_spi_iface::make(mpm::spi::spidev_iface::make(lmk_spidev));
    _clock_ctrl = boost::make_shared<lmk04828_iface>(lmk04828_iface(_clock_spi->get_write_fn(), _clock_spi->get_read_fn()));
    _mykonos_spi = mpm::spi::spidev_iface::make(mykonos_spidev);
    _mykonos_ctrl = ad937x_ctrl::make(_spi_mutex, _mykonos_spi, mpm::ad937x::gpio::gain_pins_t());
};
