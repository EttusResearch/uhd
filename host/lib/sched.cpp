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

#include <uhd/utils/static.hpp>
#include <stdexcept>
#include <iostream>

#ifdef HAVE_SCHED_H
#include <sched.h>

/*
 * # /etc/security/limits.conf
#
@usrp   -       rtprio  99
*/

UHD_STATIC_BLOCK(setup_process_sched){
    try{
        int policy = SCHED_RR;
        int max_pri = sched_get_priority_max(policy);
        if (max_pri == -1) throw std::runtime_error("sched_get_priority_max with SCHED_RR failed");
        sched_param sp; sp.sched_priority = max_pri;
        int ss_ret = sched_setscheduler(0, policy, &sp);
        if (ss_ret == -1) throw std::runtime_error("sched_setscheduler with SCHED_RR failed");
    }
    catch(const std::exception &e){
        std::cerr << "Process scheduling error: " << e.what() << std::endl;
    }
}

#endif /* HAVE_SCHED_H */
