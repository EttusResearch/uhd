//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/sensors.hpp>
#include <map>
#include <string>

using uhd::sensor_value_t;

BOOST_AUTO_TEST_CASE(test_sensor_bool) {
    auto sensor_bool = sensor_value_t(
        "bool_sensor",
        true,
        "true_unit",
        "false_unit"
    );
    BOOST_CHECK(sensor_bool.to_bool());
    BOOST_CHECK_EQUAL(sensor_bool.unit, "true_unit");

    std::map<std::string, std::string> sensor_map = {
        {"name", "bool_sensor"},
        {"type", "BOOLEAN"},
        {"unit", "true_unit"},
        {"value", "true"},
    };

    auto sensor_bool2 = sensor_value_t(sensor_map);
    BOOST_CHECK(sensor_bool2.to_bool());
    BOOST_CHECK_EQUAL(sensor_bool2.unit, "true_unit");
}


BOOST_AUTO_TEST_CASE(test_sensor_real) {
    const double sens_val = 2.25;
    const std::string sens_units = "floats";
    const std::string sens_name = "real_sensor";
    auto sensor_real = sensor_value_t(sens_name, sens_val, sens_units);
    BOOST_CHECK_EQUAL(sensor_real.to_real(), sens_val);
    BOOST_CHECK_EQUAL(sensor_real.unit, sens_units);

    std::map<std::string, std::string> sensor_map = {
        {"name", sens_name},
        {"type", "REALNUM"},
        {"unit", sens_units},
        {"value", std::to_string(sens_val)},
    };

    auto sensor_real2 = sensor_value_t(sensor_map);
    BOOST_CHECK_EQUAL(sensor_real2.to_real(), sens_val);
    BOOST_CHECK_EQUAL(sensor_real2.unit, sens_units);
}

BOOST_AUTO_TEST_CASE(test_sensor_int) {
    const int sens_val = 5;
    const std::string sens_units = "ints";
    const std::string sens_name = "int_sensor";
    auto sensor_int = sensor_value_t(sens_name, sens_val, sens_units);
    BOOST_CHECK_EQUAL(sensor_int.to_int(), sens_val);
    BOOST_CHECK_EQUAL(sensor_int.unit, sens_units);

    std::map<std::string, std::string> sensor_map = {
        {"name", sens_name},
        {"type", "INTEGER"},
        {"unit", sens_units},
        {"value", std::to_string(sens_val)},
    };

    auto sensor_int2 = sensor_value_t(sensor_map);
    BOOST_CHECK_EQUAL(sensor_int2.to_int(), sens_val);
    BOOST_CHECK_EQUAL(sensor_int2.unit, sens_units);
}

BOOST_AUTO_TEST_CASE(test_sensor_string) {
    const std::string sens_val = "foo";
    const std::string sens_units = "strings";
    const std::string sens_name = "str_sensor";
    auto sensor_str = sensor_value_t(sens_name, sens_val, sens_units);
    BOOST_CHECK_EQUAL(sensor_str.value, sens_val);
    BOOST_CHECK_EQUAL(sensor_str.unit, sens_units);

    std::map<std::string, std::string> sensor_map = {
        {"name", sens_name},
        {"type", "STRING"},
        {"unit", sens_units},
        {"value", sens_val},
    };

    auto sensor_str2 = sensor_value_t(sensor_map);
    BOOST_CHECK_EQUAL(sensor_str2.unit, sens_units);

    sensor_value_t sensor_str3(sensor_str2.to_map());
    BOOST_CHECK_EQUAL(sensor_str2.name, sensor_str3.name);
    BOOST_CHECK_EQUAL(sensor_str2.value, sensor_str3.value);
    BOOST_CHECK_EQUAL(sensor_str2.unit, sensor_str3.unit);
    BOOST_CHECK_EQUAL(sensor_str2.type, sensor_str3.type);

}

