//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0+
//

#include <uhdlib/utils/system_time.hpp>

using namespace uhd;

#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
time_spec_t uhd::get_system_time(void){
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return time_spec_t(ts.tv_sec, ts.tv_nsec, 1e9);
}
#endif /* HAVE_CLOCK_GETTIME */


#ifdef HAVE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
time_spec_t uhd::get_system_time(void){
    mach_timebase_info_data_t info; mach_timebase_info(&info);
    intmax_t nanosecs = mach_absolute_time()*info.numer/info.denom;
    return time_spec_t::from_ticks(nanosecs, 1e9);
}
#endif /* HAVE_MACH_ABSOLUTE_TIME */


#ifdef HAVE_QUERY_PERFORMANCE_COUNTER
#include <Windows.h>
time_spec_t uhd::get_system_time(void){
    LARGE_INTEGER counts, freq;
    QueryPerformanceCounter(&counts);
    QueryPerformanceFrequency(&freq);
    return time_spec_t::from_ticks(counts.QuadPart, double(freq.QuadPart));
}
#endif /* HAVE_QUERY_PERFORMANCE_COUNTER */


#ifdef HAVE_MICROSEC_CLOCK
#include <boost/date_time/posix_time/posix_time.hpp>
namespace pt = boost::posix_time;
time_spec_t uhd::get_system_time(void){
    pt::ptime time_now = pt::microsec_clock::universal_time();
    pt::time_duration time_dur = time_now - pt::from_time_t(0);
    return time_spec_t(
        time_t(time_dur.total_seconds()),
        long(time_dur.fractional_seconds()),
        double(pt::time_duration::ticks_per_second())
    );
}
#endif /* HAVE_MICROSEC_CLOCK */

