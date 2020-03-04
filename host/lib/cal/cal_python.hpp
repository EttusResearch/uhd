//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_CAL_PYTHON_HPP
#define INCLUDED_UHD_CAL_PYTHON_HPP

#include <uhd/cal/database.hpp>
#include <uhd/cal/interpolation.hpp>
#include <uhd/cal/iq_cal.hpp>

std::vector<uint8_t> pybytes_to_vector(const py::bytes& data)
{
    const std::string data_str = std::string(data);
    return std::vector<uint8_t>(data_str.cbegin(), data_str.cend());
}

py::bytes vector_to_pybytes(const std::vector<uint8_t>& data)
{
    return py::bytes(std::string(data.cbegin(), data.cend()));
}

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
        .def_static("read_cal_data",
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

    py::enum_<interp_mode>(m, "interp_mode")
        .value("NEAREST_NEIGHBOR", interp_mode::NEAREST_NEIGHBOR)
        .value("LINEAR", interp_mode::LINEAR);

    py::class_<container, std::shared_ptr<container>>(m, "container")
        .def("get_name", &container::get_name)
        .def("get_serial", &container::get_serial)
        .def("get_timestamp", &container::get_name)
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
}

#endif /* INCLUDED_UHD_CAL_PYTHON_HPP */
