//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_CAL_PYTHON_HPP
#define INCLUDED_UHD_CAL_PYTHON_HPP

#include <uhd/cal/database.hpp>

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
}

#endif /* INCLUDED_UHD_CAL_PYTHON_HPP */
