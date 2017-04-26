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

#include <mpm/types/lockable.hpp>
#include <mpm/types/regs_iface.hpp>
#include <mpm/ad937x/ad937x_ctrl.hpp>
#include <memory>
#include <mutex>

namespace mpm { namespace dboards {
    class magnesium_manager// : public dboard_periph_manager
    {
    public:
        magnesium_manager(
            const std::string &lmk_spidev,
            const std::string &mykonos_spidev
        );

        /*! Return a reference to the SPI mutex
         */
        mpm::types::lockable::sptr get_spi_lock() { return _spi_lock; }

        /*! Return a reference to the clock chip controls
         */
        mpm::types::regs_iface::sptr get_clock_ctrl(){ return _clock_ctrl; }

        /*! Return a reference to the radio chip controls
         */
        mpm::chips::ad937x_ctrl::sptr get_radio_ctrl(){ return _mykonos_ctrl; }

    private:
        std::shared_ptr<std::mutex> _spi_mutex;

        // TODO: cpld control

        mpm::types::lockable::sptr _spi_lock;
        mpm::types::regs_iface::sptr _clock_ctrl;
        mpm::chips::ad937x_ctrl::sptr _mykonos_ctrl;
    };

}}; /* namespace mpm::dboards */

#ifdef LIBMPM_PYTHON
void export_magnesium(){
    LIBMPM_BOOST_PREAMBLE("dboards")
    using namespace mpm::dboards;
    bp::class_<mpm::dboards::magnesium_manager>("magnesium_manager", bp::init<std::string, std::string>())
        .def("get_spi_lock", &mpm::dboards::magnesium_manager::get_spi_lock)
        .def("get_clock_ctrl", &mpm::dboards::magnesium_manager::get_clock_ctrl)
        .def("get_radio_ctrl", &mpm::dboards::magnesium_manager::get_radio_ctrl)
    ;
}
#endif
