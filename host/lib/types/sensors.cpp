//
// Copyright 2011-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/sensors.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>

using namespace uhd;

sensor_value_t::sensor_value_t(
    const std::string &name,
    bool value,
    const std::string &utrue,
    const std::string &ufalse
):
    name(name), value(value?"true":"false"),
    unit(value?utrue:ufalse), type(BOOLEAN)
{
    /* NOP */
}

sensor_value_t::sensor_value_t(
    const std::string &name,
    signed value,
    const std::string &unit,
    const std::string &formatter
):
    name(name), value(str(boost::format(formatter) % value)),
    unit(unit), type(INTEGER)
{
    /* NOP */
}

sensor_value_t::sensor_value_t(
    const std::string &name,
    double value,
    const std::string &unit,
    const std::string &formatter
):
    name(name), value(str(boost::format(formatter) % value)),
    unit(unit), type(REALNUM)
{
    /* NOP */
}

sensor_value_t::sensor_value_t(
    const std::string &name,
    const std::string &value,
    const std::string &unit
):
    name(name), value(value),
    unit(unit), type(STRING)
{
    /* NOP */
}

static sensor_value_t::data_type_t _string_to_type(
    const std::string &type_str
) {
    if (type_str == "STRING") {
        return sensor_value_t::STRING;
    } else if (type_str == "REALNUM") {
        return sensor_value_t::REALNUM;
    } else if (type_str == "INTEGER") {
        return sensor_value_t::INTEGER;
    } else if (type_str == "BOOLEAN") {
        return sensor_value_t::BOOLEAN;
    } else {
        throw uhd::value_error(
            std::string("Invalid sensor value type: ") + type_str
        );
    }
}

static std::string _type_to_string(
    const sensor_value_t::data_type_t &type
) {
    if (type == sensor_value_t::STRING) {
        return "STRING";
    } else if (type == sensor_value_t::REALNUM) {
        return "REALNUM";
    } else if (type == sensor_value_t::INTEGER) {
        return "INTEGER";
    } else if (type == sensor_value_t::BOOLEAN) {
        return "BOOLEAN";
    } else {
        throw uhd::value_error(
            std::string("Invalid raw sensor value type.")
        );
    }
}

sensor_value_t::sensor_value_t(
    const std::map<std::string, std::string> &sensor_dict
):
    name(sensor_dict.at("name")),
    value(sensor_dict.at("value")),
    unit(sensor_dict.at("unit")),
    type(_string_to_type(sensor_dict.at("type")))
{
    UHD_ASSERT_THROW(not name.empty());
    UHD_ASSERT_THROW(not value.empty());
    try {
        if (type == INTEGER) {
            to_int();
        } else if (type == REALNUM) {
            to_real();
        }
    }
    catch (const std::invalid_argument&) {
        throw uhd::value_error(str(
            boost::format("Could not convert sensor value `%s' to type `%s'")
            % value
            % sensor_dict.at("type")
        ));
    }
    catch (const std::out_of_range&) {
        throw uhd::value_error(str(
            boost::format("Could not convert sensor value `%s' to type `%s'")
            % value
            % sensor_dict.at("type")
        ));
    }
}

sensor_value_t::sensor_value_t(const sensor_value_t& source)
{
    *this = source;
}


std::string sensor_value_t::to_pp_string(void) const{
    switch(type){
    case BOOLEAN:
        return str(boost::format("%s: %s") % name % unit);
    case INTEGER:
    case REALNUM:
    case STRING:
        return str(boost::format("%s: %s %s") % name % value % unit);
    }
    UHD_THROW_INVALID_CODE_PATH();
}

bool sensor_value_t::to_bool(void) const{
    return value == "true";
}

signed sensor_value_t::to_int(void) const{
    return std::stoi(value);
}

double sensor_value_t::to_real(void) const{
    return std::stod(value);
}

sensor_value_t::sensor_map_t sensor_value_t::to_map(void) const{
    sensor_map_t ret_map;
    ret_map["name"] = name;
    ret_map["value"] = value;
    ret_map["unit"] = unit;
    ret_map["type"] = _type_to_string(type);
    return ret_map;
}

sensor_value_t& sensor_value_t::operator=(const sensor_value_t& rhs)
{
    this->name = rhs.name;
    this->value = rhs.value;
    this->unit = rhs.unit;
    this->type = rhs.type;
    return *this;
}
