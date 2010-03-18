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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/cstdint.hpp>

#ifndef INCLUDED_UHD_TIME_SPEC_HPP
#define INCLUDED_UHD_TIME_SPEC_HPP

namespace uhd{

    /*!
     * A time_spec_t holds a seconds and ticks time value.
     * The temporal width of a tick depends on the device's clock rate.
     * The time_spec_t can be used when setting the time on devices
     * and for controlling the start of streaming for applicable dsps.
     */
    struct time_spec_t{
        boost::uint32_t secs;
        boost::uint32_t ticks;

        /*!
         * Create a time_spec_t that holds a wildcard time.
         * This will have implementation-specific meaning.
         */
        time_spec_t(void);

        /*!
         * Create a time_spec_t from seconds and ticks.
         * \param new_secs the new seconds
         * \param new_ticks the new ticks (default = 0)
         */
        time_spec_t(boost::uint32_t new_secs, boost::uint32_t new_ticks = 0);

        /*!
         * Create a time_spec_t from boost posix time.
         * \param time fine-grained boost posix time
         * \param tick_rate the rate of ticks per second
         */
        time_spec_t(boost::posix_time::ptime time, double tick_rate);

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TIME_SPEC_HPP */
