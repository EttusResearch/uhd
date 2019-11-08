//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/ad9361/ad9361_ctrl.hpp>
#include <mpm/types/regs_iface.hpp>
#include <memory>
#include <mutex>

namespace mpm { namespace dboards {

/*! Container for the E310s AD9361 access
 */
class e31x_db_manager
{
public:
    e31x_db_manager(const std::string& catalina_spidev);

    /*! Return a reference to the RFIC controls
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
void export_e31x_db(py::module& top_module)
{
    using namespace mpm::dboards;
    auto m = top_module.def_submodule("dboards");

    py::class_<mpm::dboards::e31x_db_manager>(m, "e31x_db_manager")
        .def(py::init<std::string>())
        .def("get_radio_ctrl", &mpm::dboards::e31x_db_manager::get_radio_ctrl);
}
#endif
