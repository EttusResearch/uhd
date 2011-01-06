//
// Copyright 2011-2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/types/sensors.hpp>
#include <uhd/utils/exception.hpp>
#include <boost/format.hpp>

using namespace uhd;

sensor_value_t::sensor_value_t(
    const std::string &name,
    bool value,
    const std::string &ufalse,
    const std::string &utrue
):
    name(name), value(value?"true":"false"),
    unit(value?utrue:ufalse), type(BOOLEAN)
{
    /* NOP */
}

sensor_value_t::sensor_value_t(
    const std::string &name,
    int_type value,
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
    real_type value,
    const std::string &unit,
    const std::string &formatter
):
    name(name), value(str(boost::format(formatter) % value)),
    unit(unit), type(REALNUM)
{
    /* NOP */
}

std::string sensor_value_t::to_pp_string(void) const{
    switch(type){
    case BOOLEAN:
        return str(boost::format("%s: %s") % name % unit);
    case INTEGER:
    case REALNUM:
        return str(boost::format("%s: %s %s") % name % value % unit);
    }
    UHD_THROW_INVALID_CODE_PATH();
}
