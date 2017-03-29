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
#include "mpm/lmk04828/lmk04828_spi_iface.hpp"
#include "mpm/ad937x/ad937x_ctrl.hpp"
#include <memory>

namespace mpm { namespace dboards {
    class magnesium_periph_manager// : public dboard_periph_manager
    {
    public:
        magnesium_periph_manager(std::string lmk_spidev, std::string mykonos_spidev);

        /*! Return a reference to the clock chip
         */
        lmk04828_iface::sptr get_clock_ctrl(){return _clock_ctrl;};

        /*! Return a reference to the radio chip
         */
        ad937x_ctrl::sptr get_radio_ctrl(){return _mykonos_ctrl;};

    private:
        //cpld control
        std::shared_ptr<std::mutex> _spi_mutex;
        lmk04828_spi_iface::sptr _clock_spi;
        lmk04828_iface::sptr _clock_ctrl;
        mpm::spi::spidev_iface::sptr _mykonos_spi;
        ad937x_ctrl::sptr _mykonos_ctrl;
    };

}};


#ifdef LIBMPM_PYTHON
void export_dboards(){
    LIBMPM_BOOST_PREAMBLE("dboards")
    bp::class_<mpm::dboards::magnesium_periph_manager>("magnesium_periph_manager", bp::init<std::string, std::string>())
        .def("get_clock_ctrl", &mpm::dboards::magnesium_periph_manager::get_clock_ctrl)
        .def("get_radio_ctrl", &mpm::dboards::magnesium_periph_manager::get_radio_ctrl)
    ;
}
#endif
