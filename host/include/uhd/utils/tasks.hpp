//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_TASKS_HPP
#define INCLUDED_UHD_UTILS_TASKS_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

namespace uhd{

    class UHD_API task : boost::noncopyable{
    public:
        typedef boost::shared_ptr<task> sptr;
        typedef boost::function<void(void)> task_fcn_type;

        /*!
         * Create a new task object with function callback.
         * The task function callback will be run in a loop.
         * until the thread is interrupted by the destructor.
         *
         * A task should return in a reasonable amount of time.
         * It may not block, or the destructor will also block.
         *
         * \param task_fcn the task callback function
         * \param name Task name. Will be used as a thread name.
         * \return a new task object
         */
        static sptr make(
            const task_fcn_type &task_fcn,
            const std::string &name=""
        );
    };
} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_TASKS_HPP */

