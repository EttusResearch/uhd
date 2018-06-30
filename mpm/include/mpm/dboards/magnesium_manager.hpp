//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
            const std::string &mykonos_spidev,
            const size_t deserializer_lane_xbar
        );

        /*! Return a reference to the SPI mutex
         */
        mpm::types::lockable::sptr get_spi_lock() { return _spi_lock; }

        /*! Return a reference to the radio chip controls
         */
        mpm::chips::ad937x_ctrl::sptr get_radio_ctrl(){ return _mykonos_ctrl; }

    private:
        std::shared_ptr<std::mutex> _spi_mutex;

        // TODO: cpld control, or maybe it goes into Python

        mpm::types::lockable::sptr _spi_lock;
        mpm::chips::ad937x_ctrl::sptr _mykonos_ctrl;
    };

}}; /* namespace mpm::dboards */

#ifdef LIBMPM_PYTHON
void export_magnesium(){
    LIBMPM_BOOST_PREAMBLE("dboards")
    using namespace mpm::dboards;
    bp::class_<mpm::dboards::magnesium_manager>("magnesium_manager", bp::init<std::string,size_t>())
        .def("get_spi_lock", &mpm::dboards::magnesium_manager::get_spi_lock)
        .def("get_radio_ctrl", &mpm::dboards::magnesium_manager::get_radio_ctrl)
    ;
}
#endif
