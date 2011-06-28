//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_SENSORS_HPP
#define INCLUDED_UHD_TYPES_SENSORS_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    /*!
     * A sensor value stores a sensor reading as a string with unit and data type.
     * The sensor value class can be used in the following way:
     *
     * sensor_value_t ref_lock_sensor("Reference", my_lock, "unlocked", "locked");
     * std::cout << ref_lock_sensor.to_pp_string() << std::endl;
     * //prints Reference: locked
     *
     * sensor_value_t temp_sensor("Temperature", my_temp, "C");
     * std::cout << temp_sensor.to_pp_string() << std::endl;
     * //prints Temperature: 38.5 C
     */
    struct UHD_API sensor_value_t{

        /*!
         * Create a sensor value from a boolean.
         * \param name the name of the sensor
         * \param value the value true or false
         * \param utrue the unit string when value is true
         * \param ufalse the unit string when value is false
         */
        sensor_value_t(
            const std::string &name,
            bool value,
            const std::string &utrue,
            const std::string &ufalse
        );

        /*!
         * Create a sensor value from an integer.
         * \param name the name of the sensor
         * \param value the signed integer value
         * \param unit the associated unit type
         * \param formatter the formatter string
         */
        sensor_value_t(
            const std::string &name,
            signed value,
            const std::string &unit,
            const std::string &formatter = "%d"
        );

        /*!
         * Create a sensor value from a real number.
         * \param name the name of the sensor
         * \param value the real number value
         * \param unit the associated unit type
         * \param formatter the formatter string
         */
        sensor_value_t(
            const std::string &name,
            double value,
            const std::string &unit,
            const std::string &formatter = "%f"
        );

        /*!
         * Create a sensor value from a string.
         * \param name the name of the sensor
         * \param value the real number value
         * \param unit the associated unit type
         */
        sensor_value_t(
            const std::string &name,
            const std::string &value,
            const std::string &unit
        );

        //! convert the sensor value to a boolean
        bool to_bool(void) const;

        //! convert the sensor value to an integer
        signed to_int(void) const;

        //! convert the sensor value to real number
        double to_real(void) const;

        //! The name of the sensor value
        const std::string name;

        /*!
         * The sensor value as a string.
         * For integer and real number types, this will be the output of the formatter.
         * For boolean types, the value will be the string literal "true" or "false".
         */
        const std::string value;

        /*!
         * The sensor value's unit type.
         * For boolean types, this will be the one of the two units
         * depending upon the value of the boolean true or false.
         */
        const std::string unit;

        //! Enumeration of possible data types in a sensor
        enum data_type_t {
            BOOLEAN = 'b',
            INTEGER = 'i',
            REALNUM = 'r',
            STRING  = 's'
        };

        //! The data type of the value
        const data_type_t type;

        //! Convert this sensor value into a printable string
        std::string to_pp_string(void) const;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_SENSORS_HPP */
