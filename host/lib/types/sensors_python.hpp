//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_SENSORS_PYTHON_HPP
#define INCLUDED_UHD_SENSORS_PYTHON_HPP

#include <uhd/types/sensors.hpp>

void export_sensors()
{
    using sensor_value_t = uhd::sensor_value_t;
    using data_type_t    = sensor_value_t::data_type_t;

    bp::enum_<data_type_t>("data_type")
        .value("b", data_type_t::BOOLEAN)
        .value("i", data_type_t::INTEGER)
        .value("r", data_type_t::REALNUM)
        .value("s", data_type_t::STRING )
        ;

    bp::class_<sensor_value_t>
        ("sensor_value", bp::init<const std::string&, bool, const std::string&, const std::string&>())

        // Constructors
        .def(bp::init<const std::string&, signed, const std::string&, const std::string&>())
        .def(bp::init<const std::string&, double, const std::string&, const std::string&>())
        .def(bp::init<const std::string&, const std::string&        , const std::string&>())

        // Methods
        .def("to_bool", &sensor_value_t::to_bool     )
        .def("to_int",  &sensor_value_t::to_int      )
        .def("to_real", &sensor_value_t::to_real     )
        .def("__str__", &sensor_value_t::to_pp_string)

        // Properties
        .add_property("name",  &sensor_value_t::name )
        .add_property("value", &sensor_value_t::value)
        .add_property("unit",  &sensor_value_t::unit )
        .add_property("type",  &sensor_value_t::type )
        ;
}

#endif /* INCLUDED_UHD_SENSORS_PYTHON_HPP */
