//
// Copyright 2013 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_3000_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_3000_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/atomic.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/byteswap.hpp>
#include <queue>
#include <map>

namespace uhd{ namespace usrp{

    struct recv_packet_demuxer_3000
    {
        recv_packet_demuxer_3000(transport::zero_copy_if::sptr xport):
            _xport(xport)
        {/*NOP*/}

        transport::managed_recv_buffer::sptr get_recv_buff(const boost::uint32_t sid, const double timeout)
        {
            const time_spec_t exit_time = time_spec_t(timeout) + time_spec_t::get_system_time();
            transport::managed_recv_buffer::sptr buff;
            buff = _internal_get_recv_buff(sid, timeout);
            while (not buff) //loop until timeout
            {
                const time_spec_t delta = exit_time - time_spec_t::get_system_time();
                const double new_timeout = delta.get_real_secs();
                if (new_timeout < 0.0) break;
                buff = _internal_get_recv_buff(sid, new_timeout);
            }
            return buff;
        }

        transport::managed_recv_buffer::sptr _internal_get_recv_buff(const boost::uint32_t sid, const double timeout)
        {
            transport::managed_recv_buffer::sptr buff;

            //----------------------------------------------------------
            //-- Check the queue to see if we already have a buffer
            //----------------------------------------------------------
            {
                boost::mutex::scoped_lock l(mutex);
                queue_type_t &queue = _queues[sid];
                if (not queue.empty())
                {
                    buff = queue.front();
                    queue.front().reset();
                    queue.pop();
                    return buff;
                }
            }

            //----------------------------------------------------------
            //-- Try to claim the transport or wait patiently
            //----------------------------------------------------------
            if (_claimed.cas(1, 0))
            {
                boost::mutex::scoped_lock l(mutex);
                cond.timed_wait(l, boost::posix_time::microseconds(long(timeout*1e6)));
            }

            //----------------------------------------------------------
            //-- Wait on the transport for input buffers
            //----------------------------------------------------------
            else
            {
                buff = _xport->get_recv_buff(timeout);
                if (buff)
                {
                    const boost::uint32_t new_sid = uhd::wtohx(buff->cast<const boost::uint32_t *>()[1]);
                    if (new_sid != sid)
                    {
                        boost::mutex::scoped_lock l(mutex);
                        if (_queues.count(new_sid) == 0) UHD_MSG(error)
                            << "recv packet demuxer unexpected sid 0x" << std::hex << new_sid << std::dec
                            << std::endl;
                        else _queues[new_sid].push(buff);
                        buff.reset();
                    }
                }
                _claimed.write(0);
                cond.notify_all();
            }
            return buff;
        }

        void realloc_sid(const boost::uint32_t sid)
        {
            boost::mutex::scoped_lock l(mutex);
            while(not _queues[sid].empty()) //allocated and clears if already allocated
            {
                _queues[sid].pop();
            }
        }

        typedef std::queue<transport::managed_recv_buffer::sptr> queue_type_t;
        std::map<boost::uint32_t, queue_type_t> _queues;
        transport::zero_copy_if::sptr _xport;
        uhd::atomic_uint32_t _claimed;
        boost::condition_variable cond;
        boost::mutex mutex;
    };

}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_COMMON_RECV_PACKET_DEMUXER_3000_HPP */
