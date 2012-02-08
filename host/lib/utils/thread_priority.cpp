//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <iostream>

bool uhd::set_thread_priority_safe(float priority, bool realtime){
    try{
        set_thread_priority(priority, realtime);
        return true;
    }catch(const std::exception &e){
        UHD_MSG(warning) << boost::format(
            "Unable to set the thread priority. Performance may be negatively affected.\n"
            "Please see the general application notes in the manual for instructions.\n"
            "%s\n"
        ) % e.what();
        return false;
    }
}

static void check_priority_range(float priority){
    if (priority > +1.0 or priority < -1.0)
        throw uhd::value_error("priority out of range [-1.0, +1.0]");
}

/***********************************************************************
 * Pthread API to set priority
 **********************************************************************/
#ifdef HAVE_PTHREAD_SETSCHEDPARAM
    #include <pthread.h>

    void uhd::set_thread_priority(float priority, bool realtime){
        check_priority_range(priority);

        //when realtime is not enabled, use sched other
        int policy = (realtime)? SCHED_RR : SCHED_OTHER;

        //we cannot have below normal priority, set to zero
        if (priority < 0) priority = 0;

        //get the priority bounds for the selected policy
        int min_pri = sched_get_priority_min(policy);
        int max_pri = sched_get_priority_max(policy);
        if (min_pri == -1 or max_pri == -1) throw uhd::os_error("error in sched_get_priority_min/max");

        //set the new priority and policy
        sched_param sp;
        sp.sched_priority = int(priority*(max_pri - min_pri)) + min_pri;
        int ret = pthread_setschedparam(pthread_self(), policy, &sp);
        if (ret != 0) throw uhd::os_error("error in pthread_setschedparam");
    }
#endif /* HAVE_PTHREAD_SETSCHEDPARAM */

/***********************************************************************
 * Windows API to set priority
 **********************************************************************/
#ifdef HAVE_WIN_SETTHREADPRIORITY
    #include <windows.h>

    void uhd::set_thread_priority(float priority, bool realtime){
        check_priority_range(priority);

        /*
         * Process wide priority is no longer set.
         * This is the responsibility of the application.
        //set the priority class on the process
        int pri_class = (realtime)? REALTIME_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
        if (SetPriorityClass(GetCurrentProcess(), pri_class) == 0)
            throw uhd::os_error("error in SetPriorityClass");
         */

        //scale the priority value to the constants
        int priorities[] = {
            THREAD_PRIORITY_IDLE, THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL,
            THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL
        };
        size_t pri_index = size_t((priority+1.0)*6/2.0); // -1 -> 0, +1 -> 6

        //set the thread priority on the thread
        if (SetThreadPriority(GetCurrentThread(), priorities[pri_index]) == 0)
            throw uhd::os_error("error in SetThreadPriority");
    }
#endif /* HAVE_WIN_SETTHREADPRIORITY */

/***********************************************************************
 * Unimplemented API to set priority
 **********************************************************************/
#ifdef HAVE_THREAD_PRIO_DUMMY
    void uhd::set_thread_priority(float, bool){
        throw uhd::not_implemented_error("set thread priority not implemented");
    }

#endif /* HAVE_THREAD_PRIO_DUMMY */
