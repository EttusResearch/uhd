//
// Copyright 2011-2012 Ettus Research LLC
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

#include <uhd/types/time_spec.hpp>
#include <inttypes.h> //imaxdiv, intmax_t

using namespace uhd;

/***********************************************************************
 * Time spec system time
 **********************************************************************/

#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
time_spec_t time_spec_t::get_system_time(void){
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return time_spec_t(ts.tv_sec, ts.tv_nsec, 1e9);
}
#endif /* HAVE_CLOCK_GETTIME */


#ifdef HAVE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
time_spec_t time_spec_t::get_system_time(void){
    mach_timebase_info_data_t info; mach_timebase_info(&info);
    intmax_t nanosecs = mach_absolute_time()*info.numer/info.denom;
    return time_spec_t::from_ticks(nanosecs, 1e9);
}
#endif /* HAVE_MACH_ABSOLUTE_TIME */


#ifdef HAVE_QUERY_PERFORMANCE_COUNTER
#include <Windows.h>
time_spec_t time_spec_t::get_system_time(void){
    LARGE_INTEGER counts, freq;
    QueryPerformanceCounter(&counts);
    QueryPerformanceFrequency(&freq);
    return time_spec_t::from_ticks(counts.QuadPart, freq.QuadPart);
}
#endif /* HAVE_QUERY_PERFORMANCE_COUNTER */


#ifdef HAVE_MICROSEC_CLOCK
#include <boost/date_time/posix_time/posix_time.hpp>
namespace pt = boost::posix_time;
time_spec_t time_spec_t::get_system_time(void){
    pt::ptime time_now = pt::microsec_clock::universal_time();
    pt::time_duration time_dur = time_now - pt::from_time_t(0);
    return time_spec_t(
        time_t(time_dur.total_seconds()),
        long(time_dur.fractional_seconds()),
        double(pt::time_duration::ticks_per_second())
    );
}
#endif /* HAVE_MICROSEC_CLOCK */

/***********************************************************************
 * Time spec constructors
 **********************************************************************/
#define time_spec_init(full, frac) { \
    _full_secs = full + time_t(frac); \
    _frac_secs = frac - time_t(frac); \
    if (_frac_secs < 0) {\
        _full_secs -= 1; \
        _frac_secs += 1; \
    } \
}

UHD_INLINE long long fast_llround(const double x){
    return (long long)(x + 0.5); // assumption of non-negativity
}

time_spec_t::time_spec_t(double secs){
    time_spec_init(0, secs);
}

time_spec_t::time_spec_t(time_t full_secs, double frac_secs){
    time_spec_init(full_secs, frac_secs);
}

time_spec_t::time_spec_t(time_t full_secs, long tick_count, double tick_rate){
    const double frac_secs = tick_count/tick_rate;
    time_spec_init(full_secs, frac_secs);
}

time_spec_t time_spec_t::from_ticks(long long ticks, double tick_rate){
    const imaxdiv_t divres = imaxdiv(ticks, fast_llround(tick_rate));
    return time_spec_t(time_t(divres.quot), double(divres.rem)/tick_rate);
}

/***********************************************************************
 * Time spec accessors
 **********************************************************************/
long time_spec_t::get_tick_count(double tick_rate) const{
    return long(fast_llround(this->get_frac_secs()*tick_rate));
}

long long time_spec_t::to_ticks(double tick_rate) const{
    return fast_llround(this->get_frac_secs()*tick_rate) + \
    (this->get_full_secs() * fast_llround(tick_rate));
}

double time_spec_t::get_real_secs(void) const{
    return this->_full_secs + this->_frac_secs;
}

time_t time_spec_t::get_full_secs(void) const{
    return this->_full_secs;
}

double time_spec_t::get_frac_secs(void) const{
    return this->_frac_secs;
}

/***********************************************************************
 * Time spec math overloads
 **********************************************************************/
time_spec_t &time_spec_t::operator+=(const time_spec_t &rhs){
    time_spec_init(
        this->_full_secs + rhs.get_full_secs(),
        this->_frac_secs + rhs.get_frac_secs()
    );
    return *this;
}

time_spec_t &time_spec_t::operator-=(const time_spec_t &rhs){
    time_spec_init(
        this->_full_secs - rhs.get_full_secs(),
        this->_frac_secs - rhs.get_frac_secs()
    );
    return *this;
}

bool uhd::operator==(const time_spec_t &lhs, const time_spec_t &rhs){
    return
        lhs.get_full_secs() == rhs.get_full_secs() and
        lhs.get_frac_secs() == rhs.get_frac_secs()
    ;
}

bool uhd::operator<(const time_spec_t &lhs, const time_spec_t &rhs){
    return (
        (lhs.get_full_secs() < rhs.get_full_secs()) or (
        (lhs.get_full_secs() == rhs.get_full_secs()) and
        (lhs.get_frac_secs() < rhs.get_frac_secs())
    ));
}
