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
#include <boost/operators.hpp>

namespace uhd{

    /*!
     * A time_spec_t holds a seconds and fractional seconds time value.
     * The time_spec_t can be used when setting the time on devices,
     * and for dealing with time stamped samples though the metadata.
     * and for controlling the start of streaming for applicable dsps.
     *
     * The fractional seconds are represented in units of nanoseconds,
     * which provide a clock-domain independent unit of time storage.
     * The methods "get_ticks" and "set_ticks" can be used to convert
     * the fractional seconds to and from clock-domain specific units.
     *
     * The nanoseconds count is stored as double precision floating point.
     * This gives the fractional seconds enough precision to unambiguously
     * specify a clock-tick/sample-count up to rates of several petahertz.
     */
    struct UHD_API time_spec_t:
        boost::addable<time_spec_t>,
        boost::subtractable<time_spec_t>,
        boost::equality_comparable<time_spec_t>{

        //! whole/integer seconds count in seconds
        boost::uint32_t secs;

        //! fractional seconds count in nano-seconds
        double nsecs;

        /*!
         * Convert the fractional nsecs to clock ticks.
         * Translation into clock-domain specific units.
         * \param tick_rate the number of ticks per second
         * \return the fractional seconds tick count
         */
        boost::uint32_t get_ticks(double tick_rate) const;

        /*!
         * Set the fractional nsecs from clock ticks.
         * Translation from clock-domain specific units.
         * \param ticks the fractional seconds tick count
         * \param tick_rate the number of ticks per second
         */
        void set_ticks(boost::uint32_t ticks, double tick_rate);

        /*!
         * Create a time_spec_t from whole and fractional seconds.
         * \param secs the whole/integer seconds count in seconds (default = 0)
         * \param nsecs the fractional seconds count in nanoseconds (default = 0)
         */
        time_spec_t(boost::uint32_t secs = 0, double nsecs = 0);

        /*!
         * Create a time_spec_t from whole and fractional seconds.
         * Translation from clock-domain specific units.
         * \param secs the whole/integer seconds count in seconds
         * \param ticks the fractional seconds tick count
         * \param tick_rate the number of ticks per second
         */
        time_spec_t(boost::uint32_t secs, boost::uint32_t ticks, double tick_rate);

        //! Implement addable interface
        time_spec_t &operator+=(const time_spec_t &);

        //! Implement subtractable interface
        time_spec_t &operator-=(const time_spec_t &);

    };

    //! Implement equality_comparable interface
    UHD_API bool operator==(const time_spec_t &, const time_spec_t &);

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TIME_SPEC_HPP */
