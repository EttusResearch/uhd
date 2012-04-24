//
// Copyright 2011-2012 Ettus Research LLC
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
         * until the thread is interrupted by the deconstructor.
         *
         * A task should return in a reasonable amount of time
         * or may block forever under the following conditions:
         *  - The blocking call is interruptible.
         *  - The task polls the interrupt condition.
         *
         * \param task_fcn the task callback function
         * \return a new task object
         */
        static sptr make(const task_fcn_type &task_fcn);

    };

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_TASKS_HPP */

