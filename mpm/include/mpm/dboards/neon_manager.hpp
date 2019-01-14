//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/ad9361/ad9361_ctrl.hpp>
#include <mpm/types/lockable.hpp>
#include <mpm/types/regs_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <mutex>

namespace mpm { namespace dboards {
class neon_manager // : public dboard_periph_manager
{
public:
    neon_manager(const std::string& catalina_spidev);

    /*! Return a reference to the radio chip controls
     */
    mpm::chips::ad9361_ctrl::sptr get_radio_ctrl()
    {
        return _catalina_ctrl;
    }

private:
    mpm::chips::ad9361_ctrl::sptr _catalina_ctrl;
};

}}; /* namespace mpm::dboards */

#ifdef LIBMPM_PYTHON
void export_neon()
{
    LIBMPM_BOOST_PREAMBLE("dboards")
    using namespace mpm::dboards;
    bp::class_<mpm::dboards::neon_manager>("neon_manager", bp::init<std::string>())
        .def("get_radio_ctrl", &mpm::dboards::neon_manager::get_radio_ctrl);
}
#endif
