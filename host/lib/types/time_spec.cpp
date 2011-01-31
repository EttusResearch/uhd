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

#include <uhd/types/time_spec.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;

/***********************************************************************
 * Time spec system time
 **********************************************************************/

/*!
 * Creates a time spec from system counts:
 * TODO make part of API as a static factory function
 * The counts type is 64 bits and will overflow the ticks type of long.
 * Therefore, divmod the counts into seconds + sub-second counts first.
 */
#include <inttypes.h> //imaxdiv, intmax_t
static UHD_INLINE time_spec_t time_spec_t_from_counts(intmax_t counts, intmax_t freq){
    imaxdiv_t divres = imaxdiv(counts, freq);
    return time_spec_t(time_t(divres.quot), double(divres.rem)/freq);
}

#ifdef TIME_SPEC_USE_CLOCK_GETTIME
#include <time.h>
time_spec_t time_spec_t::get_system_time(void){
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return time_spec_t(ts.tv_sec, ts.tv_nsec, 1e9);
}
#endif /* TIME_SPEC_USE_CLOCK_GETTIME */


#ifdef TIME_SPEC_USE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
static intmax_t get_freq(void){
    mach_timebase_info_data_t info; mach_timebase_info(&info);
    return intmax_t(1e9*(double(info.denom)/double(info.numer)));
}
time_spec_t time_spec_t::get_system_time(void){
    static const intmax_t freq = get_freq();
    return time_spec_t_from_counts(mach_absolute_time(), freq);
}
#endif /* TIME_SPEC_USE_MACH_ABSOLUTE_TIME */


#ifdef TIME_SPEC_USE_QUERY_PERFORMANCE_COUNTER
#include <Windows.h>
time_spec_t time_spec_t::get_system_time(void){
    LARGE_INTEGER counts, freq;
    QueryPerformanceCounter(&counts);
    QueryPerformanceFrequency(&freq);
    return time_spec_t_from_counts(counts.QuadPart, freq.QuadPart);
}
#endif /* TIME_SPEC_USE_QUERY_PERFORMANCE_COUNTER */


#ifdef TIME_SPEC_USE_MICROSEC_CLOCK
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
#endif /* TIME_SPEC_USE_MICROSEC_CLOCK */

/***********************************************************************
 * Time spec constructors
 **********************************************************************/
time_spec_t::time_spec_t(double secs):
    _full_secs(0),
    _frac_secs(secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, double frac_secs):
    _full_secs(full_secs),
    _frac_secs(frac_secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, long tick_count, double tick_rate):
    _full_secs(full_secs),
    _frac_secs(double(tick_count)/tick_rate)
{
    /* NOP */
}

/***********************************************************************
 * Time spec accessors
 **********************************************************************/
long time_spec_t::get_tick_count(double tick_rate) const{
    return boost::math::iround(this->get_frac_secs()*tick_rate);
}

double time_spec_t::get_real_secs(void) const{
    return this->_full_secs + this->_frac_secs;
}

time_t time_spec_t::get_full_secs(void) const{
    double intpart;
    std::modf(this->_frac_secs, &intpart);
    return this->_full_secs + time_t(intpart);
}

double time_spec_t::get_frac_secs(void) const{
    return std::fmod(this->_frac_secs, 1.0);
}

/***********************************************************************
 * Time spec math overloads
 **********************************************************************/
time_spec_t &time_spec_t::operator+=(const time_spec_t &rhs){
    this->_full_secs += rhs.get_full_secs();
    this->_frac_secs += rhs.get_frac_secs();
    return *this;
}

time_spec_t &time_spec_t::operator-=(const time_spec_t &rhs){
    this->_full_secs -= rhs.get_full_secs();
    this->_frac_secs -= rhs.get_frac_secs();
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
