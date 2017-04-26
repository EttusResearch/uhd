//
// Copyright 2017 Ettus Research (National Instruments)
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

