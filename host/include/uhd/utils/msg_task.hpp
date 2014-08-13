//
// Copyright 2011-2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_MSG_TASK_HPP
#define INCLUDED_UHD_UTILS_MSG_TASK_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/optional/optional.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd{
	class UHD_API msg_task : boost::noncopyable{
        public:
            typedef boost::shared_ptr<msg_task> sptr;
            typedef std::vector<boost::uint8_t> msg_payload_t;
            typedef std::pair<boost::uint32_t, msg_payload_t > msg_type_t;
            typedef boost::function<boost::optional<msg_type_t>(void)> task_fcn_type;

            /*
             * During shutdown message queues for radio control cores might not be available anymore.
             * Such stranded messages get pushed into a dump queue.
             * With this function radio_ctrl_core can check if one of the messages meant for it got stranded.
             */
            virtual msg_payload_t get_msg_from_dump_queue(boost::uint32_t sid) = 0;

            inline static std::vector<boost::uint8_t> buff_to_vector(boost::uint8_t* p, size_t n) {
                if(p and n > 0){
                    std::vector<boost::uint8_t> v(n);
                    memcpy(&v.front(), p, n);
                    return v;
                }
                return std::vector<boost::uint8_t>();
            }

            virtual ~msg_task(void) = 0;

            /*!
             * Create a new task object with function callback.
             * The task function callback will be run in a loop.
             * until the thread is interrupted by the deconstructor.
             *
             * A function may return payload which is then pushed to
             * a synchronized message queue.
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

#endif /* INCLUDED_UHD_UTILS_MSG_TASK_HPP */

