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

#pragma once

#include "mpm/spi/spidev_iface.hpp"
#include "lmk/lmk04828_spi_iface.hpp"
#include <memory>

namespace mpm { namespace n3xx {
        /*
        class dboard_periph_manager
        {
            //....
        };
        */
    /**************************************************************************
     * Daughterboard peripherals
     *************************************************************************/

    /*! Encapsulates all peripherals stored on the regular N300/N310
     *  daughterboard.
     */
    class n3xx_dboard_periph_manager// : public dboard_periph_manager
    {
    public:
        n3xx_dboard_periph_manager(
                uhd::spi_iface::sptr lmk_spi
            //  uhd::spi_iface::sptr myk_spi
            //  uhd::spi_iface::sptr cpld_spi

            );

        /*! Return a reference to the clock chip
         */
        lmk04828_iface::sptr get_clock_gen();

    private:
        //ad937x_ctrl::sptr _rfic;
        //cpld control
        lmk04828_iface::sptr _clock_gen;
        std::shared_ptr<lmk04828_spi_iface> _clock_spi;
    };

    /**************************************************************************
     * Motherboard peripherals
     *************************************************************************/
    class periph_manager
    {
    public:

        periph_manager(std::string spi_path): dboard_A_manager(mpm::spi::spidev_iface::make(spi_path))
        {

        };

        n3xx_dboard_periph_manager get_dboard_A();
        lmk04828_iface::sptr get_clock_gen();

        // virtual void set_clock_source();

    private:
        n3xx_dboard_periph_manager dboard_A_manager;
  //      n3xx_dboard_periph_manager dboard_B_manager;
    };

}}; /* namespace mpm::n310 */

