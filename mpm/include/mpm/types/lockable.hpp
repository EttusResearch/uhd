//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>

namespace mpm { namespace types {

    /*! A lockable object
     *
     * Don't tell anyone, but's really just a wrapper around a mutex. This
     * class is primarily to make it easy to safely expose that mutex into
     * Python.
     */
    class lockable : public boost::noncopyable
    {
    public:
        using sptr = std::shared_ptr<lockable>;

        /*! Lock the lock
         */
        virtual void lock() = 0;

        /*! Unlock the lock
         */
        virtual void unlock() = 0;

        static sptr make(
            std::shared_ptr<std::mutex> spi_mutex
        );
    };

}}; /* namespace mpm::types */

