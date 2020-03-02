//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_SENSORS_PYTHON_HPP
#define INCLUDED_UHD_SENSORS_PYTHON_HPP

#include <uhd/types/sensors.hpp>

void export_sensors(py::module& m)
{
    using sensor_value_t = uhd::sensor_value_t;
    using data_type_t    = sensor_value_t::data_type_t;

    py::enum_<data_type_t>(m, "data_type")
        .value("b", data_type_t::BOOLEAN)
        .value("i", data_type_t::INTEGER)
        .value("r", data_type_t::REALNUM)
        .value("s", data_type_t::STRING);

    py::class_<sensor_value_t>(m, "sensor_value")
        // Constructors
        .def(py::init<const std::string&, bool, const std::string&, const std::string&>())
        .def(py::init<const std::string&,
            signed,
            const std::string&,
            const std::string&>())
        .def(py::init<const std::string&,
            double,
            const std::string&,
            const std::string&>())
        .def(py::init<const std::string&, const std::string&, const std::string&>())

        // Methods
        .def("to_bool", &sensor_value_t::to_bool)
        .def("to_int", &sensor_value_t::to_int)
        .def("to_real", &sensor_value_t::to_real)
        .def("__str__", &sensor_value_t::to_pp_string)

        // Properties
        .def_readwrite("name", &sensor_value_t::name)
        .def_readwrite("value", &sensor_value_t::value)
        .def_readwrite("unit", &sensor_value_t::unit)
        .def_readwrite("type", &sensor_value_t::type);
}

#endif /* INCLUDED_UHD_SENSORS_PYTHON_HPP */
