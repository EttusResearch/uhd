//
// Copyright 2012 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_TRANSPORT_SIMPLE_CLAIMER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_SIMPLE_CLAIMER_HPP

#include <uhd/config.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace uhd{ namespace transport{

/***********************************************************************
 * Claimer class to provide synchronization for multi-thread access.
 * Claiming enables buffer classes to be used with a buffer queue.
 **********************************************************************/
class simple_claimer{
public:
    simple_claimer(void){
        this->release();
    }

    UHD_INLINE void release(void){
        boost::mutex::scoped_lock lock(_mutex);
        _locked = false;
        lock.unlock();
        _cond.notify_one();
    }

    UHD_INLINE bool claim_with_wait(const double timeout){
        boost::mutex::scoped_lock lock(_mutex);
        while (_locked){
            if (not _cond.timed_wait(lock, boost::posix_time::microseconds(long(timeout*1e6)))){
                break;
            }
        }
        const bool ret = not _locked;
        _locked = true;
        return ret;
    }

private:
    bool _locked;
    boost::mutex _mutex;
    boost::condition_variable _cond;
};

}} //namespace uhd::transport

#endif /* INCLUDED_LIBUHD_TRANSPORT_SIMPLE_CLAIMER_HPP */
