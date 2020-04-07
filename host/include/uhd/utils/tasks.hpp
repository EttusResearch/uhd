//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2017 Ettus Research (National Instruments Corp.)
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>

namespace uhd {

class UHD_API task : uhd::noncopyable
{
public:
    typedef std::shared_ptr<task> sptr;
    typedef std::function<void(void)> task_fcn_type;

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
    static sptr make(const task_fcn_type& task_fcn, const std::string& name = "");
};
} // namespace uhd
