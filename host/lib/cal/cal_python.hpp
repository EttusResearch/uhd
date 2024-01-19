//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_CAL_PYTHON_HPP
#define INCLUDED_UHD_CAL_PYTHON_HPP

#include <uhd/cal/database.hpp>
#include <uhd/cal/dsa_cal.hpp>
#include <uhd/cal/iq_cal.hpp>
#include <uhd/cal/pwr_cal.hpp>
#include <uhd/utils/interpolation.hpp>
#include <uhd/utils/pybind_adaptors.hpp>
#include <pybind11/stl.h>

void export_cal(py::module& m)
{
    using namespace uhd::usrp::cal;

    // Cal Database
    using database = uhd::usrp::cal::database;
    using source   = uhd::usrp::cal::source;

    py::enum_<source>(m, "source")
        .value("ANY", source::ANY)
        .value("RC", source::RC)
        .value("FILESYSTEM", source::FILESYSTEM)
        .value("FLASH", source::FLASH)
        .value("USER", source::USER)
        .value("NONE", source::NONE);

    py::class_<database>(m, "database")
        .def_static(
            "read_cal_data",
            [](const std::string& key,
                const std::string& serial,
                const source source_type) {
                return vector_to_pybytes(
                    database::read_cal_data(key, serial, source_type));
            },
            py::arg("key"),
            py::arg("serial"),
            py::arg("source_type") = source::ANY)
        .def_static("has_cal_data",
            &database::has_cal_data,
            py::arg("key"),
            py::arg("serial"),
            py::arg("source_type") = source::ANY)
        .def_static("write_cal_data",
            [](const std::string& key, const std::string& serial, const py::bytes data) {
                database::write_cal_data(key, serial, pybytes_to_vector(data));
            });

    py::enum_<uhd::math::interp_mode>(m, "interp_mode")
        .value("NEAREST_NEIGHBOR", uhd::math::interp_mode::NEAREST_NEIGHBOR)
        .value("LINEAR", uhd::math::interp_mode::LINEAR);

    py::class_<container, std::shared_ptr<container>>(m, "container")
        .def("get_name", &container::get_name)
        .def("get_serial", &container::get_serial)
        .def("get_timestamp", &container::get_timestamp)
        .def("serialize",
            [](std::shared_ptr<container>& self) {
                return vector_to_pybytes(self->serialize());
            })
        .def("deserialize", [](std::shared_ptr<container>& self, const py::bytes data) {
            self->deserialize(pybytes_to_vector(data));
        });

    py::class_<iq_cal, container, iq_cal::sptr>(m, "iq_cal")
        .def(py::init([](const std::string& name,
                          const std::string& serial,
                          const uint64_t timestamp) {
            return iq_cal::make(name, serial, timestamp);
        }))
        .def(py::init([]() { return iq_cal::make(); }))
        .def(py::init([](const py::bytes data) {
            return container::make<iq_cal>(pybytes_to_vector(data));
        }))
        .def("set_interp_mode", &iq_cal::set_interp_mode)
        .def("get_cal_coeff", &iq_cal::get_cal_coeff)
        .def("set_cal_coeff",
            &iq_cal::set_cal_coeff,
            py::arg("freq"),
            py::arg("coeff"),
            py::arg("suppression_abs")   = 0,
            py::arg("suppression_delta") = 0)
        .def("clear", &iq_cal::clear);

    py::class_<pwr_cal, container, pwr_cal::sptr>(m, "pwr_cal")
        .def(py::init([](const std::string& name,
                          const std::string& serial,
                          const uint64_t timestamp) {
            return pwr_cal::make(name, serial, timestamp);
        }))
        .def(py::init([]() { return pwr_cal::make(); }))
        .def(py::init([](const py::bytes data) {
            return container::make<pwr_cal>(pybytes_to_vector(data));
        }))
        .def("add_power_table",
            &pwr_cal::add_power_table,
            py::arg("gain_power_map"),
            py::arg("min_power"),
            py::arg("max_power"),
            py::arg("freq"),
            py::arg("temperature") = boost::optional<int>())
        .def("clear", &pwr_cal::clear)
        .def("set_temperature", &pwr_cal::set_temperature)
        .def("get_temperature", &pwr_cal::get_temperature)
        .def("set_ref_gain", &pwr_cal::set_ref_gain)
        .def("get_ref_gain", &pwr_cal::get_ref_gain)
        .def("get_power_limits",
            &pwr_cal::get_power_limits,
            py::arg("freq"),
            py::arg("temperature") = boost::optional<int>())
        .def("get_power",
            &pwr_cal::get_power,
            py::arg("gain"),
            py::arg("freq"),
            py::arg("temperature") = boost::optional<int>())
        .def("get_gain",
            &pwr_cal::get_gain,
            py::arg("power_dbm"),
            py::arg("freq"),
            py::arg("temperature") = boost::optional<int>());

    py::class_<zbx_tx_dsa_cal, container, zbx_tx_dsa_cal::sptr>(m, "zbx_tx_dsa_cal")
        .def(py::init([](const std::string& name,
                          const std::string& serial,
                          const uint64_t timestamp) {
            return zbx_tx_dsa_cal::make(name, serial, timestamp);
        }))
        .def(py::init([]() { return zbx_tx_dsa_cal::make(); }))
        .def(py::init([](const py::bytes data) {
            return container::make<zbx_tx_dsa_cal>(pybytes_to_vector(data));
        }))
        .def("add_frequency_band",
            &zbx_tx_dsa_cal::add_frequency_band,
            py::arg("max_freq"),
            py::arg("name"),
            py::arg("steps"))
        .def("clear", &zbx_tx_dsa_cal::clear)
        .def("get_dsa_setting",
            &zbx_tx_dsa_cal::get_dsa_setting,
            py::arg("freq"),
            py::arg("gain_index"));

    py::class_<zbx_rx_dsa_cal, container, zbx_rx_dsa_cal::sptr>(m, "zbx_rx_dsa_cal")
        .def(py::init([](const std::string& name,
                          const std::string& serial,
                          const uint64_t timestamp) {
            return zbx_rx_dsa_cal::make(name, serial, timestamp);
        }))
        .def(py::init([]() { return zbx_rx_dsa_cal::make(); }))
        .def(py::init([](const py::bytes data) {
            return container::make<zbx_rx_dsa_cal>(pybytes_to_vector(data));
        }))
        .def("add_frequency_band",
            &zbx_rx_dsa_cal::add_frequency_band,
            py::arg("max_freq"),
            py::arg("name"),
            py::arg("steps"))
        .def("clear", &zbx_rx_dsa_cal::clear)
        .def("get_dsa_setting",
            &zbx_rx_dsa_cal::get_dsa_setting,
            py::arg("freq"),
            py::arg("gain_index"));
}

#endif /* INCLUDED_UHD_CAL_PYTHON_HPP */
