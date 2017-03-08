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

#include "periph_manager.hpp"

using namespace mpm::n3xx;

n3xx_dboard_periph_manager::n3xx_dboard_periph_manager(
            uhd::spi_iface::sptr lmk_spi
            //uhd::spi_iface::sptr myk_spi
            //uhd::spi_iface::sptr cpld_spi
        )
    {
        _clock_spi = std::make_shared<lmk04828_spi_iface>(lmk04828_spi_iface(lmk_spi));
        _clock_gen = lmk04828_iface::make(_clock_spi->get_write_fn(), _clock_spi->get_read_fn());
    };

lmk04828_iface::sptr n3xx_dboard_periph_manager::get_clock_gen() { return _clock_gen; }

n3xx_dboard_periph_manager periph_manager::get_dboard_A(){
    return dboard_A_manager;
}

lmk04828_iface::sptr periph_manager::get_clock_gen(){ return dboard_A_manager.get_clock_gen(); }
