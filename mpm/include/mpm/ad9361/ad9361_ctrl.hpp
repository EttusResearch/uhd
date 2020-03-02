//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// Relative to uhd/host/lib/usrp/common/ad9361_driver/
#include "../../../include/uhdlib/usrp/common/ad9361_ctrl.hpp"
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <functional>
#include <future>
#include <string>
#include <vector>

namespace mpm { namespace chips {
using uhd::usrp::ad9361_ctrl;
}}; // namespace mpm::chips

//! Async calls
std::future<double> handle_tune;
std::future<double> handle_set_clock_rate;

// TODO: pull in filter_info_base
#ifdef LIBMPM_PYTHON
void export_catalina(py::module& top_module)
{
    using namespace mpm::chips;
    auto m = top_module.def_submodule("ad9361");

    py::class_<ad9361_ctrl, boost::shared_ptr<ad9361_ctrl>>(m, "ad9361_ctrl")
        .def_static("get_gain_names", &ad9361_ctrl::get_gain_names)
        // Make this "Python private" because the return value can't be serialized
        .def_static("_get_gain_range", &ad9361_ctrl::get_gain_range)
        .def_static("get_rf_freq_range", &ad9361_ctrl::get_rf_freq_range)
        .def_static("get_bw_filter_range", &ad9361_ctrl::get_bw_filter_range)
        .def_static("get_clock_rate_range", &ad9361_ctrl::get_clock_rate_range)
        .def("set_bw_filter", &ad9361_ctrl::set_bw_filter)
        .def("set_gain", &ad9361_ctrl::set_gain)
        .def("set_agc", &ad9361_ctrl::set_agc)
        .def("set_agc_mode", &ad9361_ctrl::set_agc_mode)
        .def("set_clock_rate", &ad9361_ctrl::set_clock_rate)
        .def("set_active_chains", &ad9361_ctrl::set_active_chains)
        .def("set_timing_mode", &ad9361_ctrl::set_timing_mode)
        .def("tune", &ad9361_ctrl::tune)
        .def("set_dc_offset", &ad9361_ctrl::set_dc_offset)
        .def("set_dc_offset_auto", &ad9361_ctrl::set_dc_offset_auto)
        .def("set_iq_balance", &ad9361_ctrl::set_iq_balance)
        .def("set_iq_balance_auto", &ad9361_ctrl::set_iq_balance_auto)
        .def("get_freq", &ad9361_ctrl::get_freq)
        .def("data_port_loopback", &ad9361_ctrl::data_port_loopback)
        .def("get_rssi",
            +[](ad9361_ctrl& self, std::string which) {
                return self.get_rssi(which).to_real();
            })
        .def("get_temperature",
            +[](ad9361_ctrl& self) { return self.get_temperature().to_real(); })
        .def("get_filter_names", &ad9361_ctrl::get_filter_names)
        // Make this "Python private" because the return value can't be serialized.
        .def("_get_filter", &ad9361_ctrl::get_filter)
        .def("set_filter", &ad9361_ctrl::set_filter)
        .def("output_digital_test_tone", &ad9361_ctrl::output_digital_test_tone);

    m.def("async__tune",
        +[](ad9361_ctrl& catalina, const std::string& which, const double value) {
            handle_tune = std::async(
                std::launch::async, &ad9361_ctrl::tune, &catalina, which, value);
        });
    m.def("await__tune", +[]() -> bool {
        if (handle_tune.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            handle_tune.get();
            return true;
        }
        return false;
    });
    m.def("async__set_clock_rate", +[](ad9361_ctrl& catalina, const double value) {
        handle_set_clock_rate = std::async(
            std::launch::async, &ad9361_ctrl::set_clock_rate, &catalina, value);
    });
    m.def("await__set_clock_rate", +[]() -> bool {
        if (handle_set_clock_rate.wait_for(std::chrono::seconds(0))
            == std::future_status::ready) {
            handle_set_clock_rate.get();
            return true;
        }
        return false;
    });
}

#endif
