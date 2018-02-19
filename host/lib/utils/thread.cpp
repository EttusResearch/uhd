//
// Copyright 2010-2011,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <iostream>

bool uhd::set_thread_priority_safe(float priority, bool realtime){
    try{
        set_thread_priority(priority, realtime);
        return true;
    }catch(const std::exception &e){
        UHD_LOGGER_WARNING("UHD") << boost::format(
            "Unable to set the thread priority. Performance may be negatively affected.\n"
            "Please see the general application notes in the manual for instructions.\n"
            "%s"
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

    void uhd::set_thread_priority(float priority, UHD_UNUSED(bool realtime)){
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

void uhd::set_thread_name(
    boost::thread *thrd,
    const std::string &name
) {
#ifdef HAVE_PTHREAD_SETNAME
    pthread_setname_np(thrd->native_handle(), name.substr(0,16).c_str());
#endif /* HAVE_PTHREAD_SETNAME */
#ifdef HAVE_THREAD_SETNAME_DUMMY
    UHD_LOG_DEBUG("UHD", "Setting thread name is not implemented; wanted to set to " << name);
#endif /* HAVE_THREAD_SETNAME_DUMMY */
}
