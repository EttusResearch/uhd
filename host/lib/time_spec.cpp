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

#include <uhd/time_spec.hpp>

using namespace uhd;

time_spec_t::time_spec_t(boost::uint32_t new_secs, boost::uint32_t new_ticks){
    secs = new_secs;
    ticks = new_ticks;
}

static const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
static double time_tick_rate = double(boost::posix_time::time_duration::ticks_per_second());

time_spec_t::time_spec_t(boost::posix_time::ptime time, double tick_rate){
    boost::posix_time::time_duration td = time - epoch;
    secs = boost::uint32_t(td.total_seconds());
    double time_ticks_per_device_ticks = time_tick_rate/tick_rate;
    ticks = boost::uint32_t(td.fractional_seconds()/time_ticks_per_device_ticks);
}
