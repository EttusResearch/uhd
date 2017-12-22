//
// Copyright 2011-2011 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
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

sensor_value_t& sensor_value_t::operator=(const sensor_value_t& rhs)
{
    this->name = rhs.name;
    this->value = rhs.value;
    this->unit = rhs.unit;
    this->type = rhs.type;
    return *this;
}
