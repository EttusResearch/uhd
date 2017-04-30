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
#include <memory>
#include <mutex>

namespace mpm { namespace dboards {
    class eiscat_manager// : public dboard_periph_manager
    {
    public:
        eiscat_manager(
            const std::string &lmk_spidev,
            const std::string &adc0_spidev,
            const std::string &adc1_spidev
            //const std::string &phdac_spidev,
        );

        /*! Return a reference to the SPI mutex
         */
        mpm::types::lockable::sptr get_spi_lock() { return _spi_lock; }

        /*! Return a reference to the clock chip controls
         */
        mpm::types::regs_iface::sptr get_clock_ctrl(){ return _clock_ctrl; }

        mpm::types::regs_iface::sptr get_adc0_ctrl(){ return _adc0_ctrl; }
        mpm::types::regs_iface::sptr get_adc1_ctrl(){ return _adc1_ctrl; }

    private:
        std::shared_ptr<std::mutex> _spi_mutex;

        mpm::types::lockable::sptr _spi_lock;
        mpm::types::regs_iface::sptr _clock_ctrl;
        mpm::types::regs_iface::sptr _adc0_ctrl;
        mpm::types::regs_iface::sptr _adc1_ctrl;
        //mpm::types::regs_iface::sptr _phdac_ctrl;
    };

}}; /* namespace mpm::dboards */

#ifdef LIBMPM_PYTHON
void export_eiscat(){
    LIBMPM_BOOST_PREAMBLE("eiscat")
    using namespace mpm::dboards;
    bp::class_<mpm::dboards::eiscat_manager>("eiscat_manager", bp::init<std::string, std::string, std::string>())
        .def("get_spi_lock", &mpm::dboards::eiscat_manager::get_spi_lock)
        .def("get_clock_ctrl", &mpm::dboards::eiscat_manager::get_clock_ctrl)
        .def("get_adc0_ctrl", &mpm::dboards::eiscat_manager::get_adc0_ctrl)
        .def("get_adc1_ctrl", &mpm::dboards::eiscat_manager::get_adc1_ctrl)
    ;
}
#endif
