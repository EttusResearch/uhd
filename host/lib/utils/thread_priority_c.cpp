//
// Copyright 2015 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#include <uhd/error.h>
#include <uhd/utils/thread_priority.h>
#include <uhd/utils/thread.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <iostream>

uhd_error uhd_set_thread_priority(
    float priority,
    bool realtime
){
    UHD_SAFE_C(
        uhd::set_thread_priority(priority, realtime);
    )
}
