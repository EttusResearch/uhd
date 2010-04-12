//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_TIME_SPEC_HPP
#define INCLUDED_UHD_TYPES_TIME_SPEC_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>

namespace uhd{

    /*!
     * A time_spec_t holds a seconds and fractional seconds time value.
     * The time_spec_t can be used when setting the time on devices,
     * and for dealing with time stamped samples though the metadata.
     * and for controlling the start of streaming for applicable dsps.
     */
    struct UHD_API time_spec_t{

        //! whole seconds count
        boost::uint32_t secs;

        //! fractional seconds count in nano-seconds
        double nsecs;

        /*!
         * Convert the fractional nsecs to clock ticks.
         * \param tick_rate the number of ticks per second
         * \return the number of ticks in this time spec
         */
        boost::uint32_t get_ticks(double tick_rate) const;

        /*!
         * Set the fractional nsecs from clock ticks.
         * \param ticks the fractional seconds tick count
         * \param tick_rate the number of ticks per second
         */
        void set_ticks(boost::uint32_t ticks, double tick_rate);

        /*!
         * Create a time_spec_t from seconds and ticks.
         * \param new_secs the new seconds (default = 0)
         * \param new_nsecs the new nano-seconds (default = 0)
         */
        time_spec_t(boost::uint32_t new_secs = 0, double new_nsecs = 0);

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TIME_SPEC_HPP */
