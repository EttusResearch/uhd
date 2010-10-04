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

#ifndef INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_IPP
#define INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_IPP

#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace uhd{ namespace transport{ namespace{ /*anon*/

    template <typename elem_type>
    class bounded_buffer_impl : public bounded_buffer<elem_type>{
    public:

        bounded_buffer_impl(size_t capacity) : _buffer(capacity){
            /* NOP */
        }

        UHD_INLINE bool push_with_pop_on_full(const elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if(_buffer.full()){
                _buffer.pop_back();
                _buffer.push_front(elem);
                lock.unlock();
                _empty_cond.notify_one();
                return false;
            }
            else{
                _buffer.push_front(elem);
                lock.unlock();
                _empty_cond.notify_one();
                return true;
            }
        }

        UHD_INLINE void push_with_wait(const elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            _full_cond.wait(lock, boost::bind(&bounded_buffer_impl<elem_type>::not_full, this));
            _buffer.push_front(elem);
            lock.unlock();
            _empty_cond.notify_one();
        }

        bool push_with_timed_wait(const elem_type &elem, double timeout){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if (not _full_cond.timed_wait(
                lock, boost::posix_time::microseconds(long(timeout*1e6)),
                boost::bind(&bounded_buffer_impl<elem_type>::not_full, this)
            )) return false;
            _buffer.push_front(elem);
            lock.unlock();
            _empty_cond.notify_one();
            return true;
        }

        UHD_INLINE void pop_with_wait(elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            _empty_cond.wait(lock, boost::bind(&bounded_buffer_impl<elem_type>::not_empty, this));
            this->pop_back(elem);
            lock.unlock();
            _full_cond.notify_one();
        }

        bool pop_with_timed_wait(elem_type &elem, double timeout){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if (not _empty_cond.timed_wait(
                lock, boost::posix_time::microseconds(long(timeout*1e6)),
                boost::bind(&bounded_buffer_impl<elem_type>::not_empty, this)
            )) return false;
            this->pop_back(elem);
            lock.unlock();
            _full_cond.notify_one();
            return true;
        }

        UHD_INLINE void clear(void){
            boost::unique_lock<boost::mutex> lock(_mutex);
            while (not_empty()) _buffer.pop_back();
            lock.unlock();
            _full_cond.notify_one();
        }

    private:
        boost::mutex _mutex;
        boost::condition _empty_cond, _full_cond;
        boost::circular_buffer<elem_type> _buffer;

        bool not_full(void) const{return not _buffer.full();}
        bool not_empty(void) const{return not _buffer.empty();}

        /*!
         * Three part operation to pop an element:
         * 1) assign elem to the back element
         * 2) assign the back element to empty
         * 3) pop the back to move the counter
         */
        UHD_INLINE void pop_back(elem_type &elem){
            elem = _buffer.back();
            _buffer.back() = elem_type();
            _buffer.pop_back();
        }
    };
}}} //namespace

namespace uhd{ namespace transport{

    template <typename elem_type> typename bounded_buffer<elem_type>::sptr
    bounded_buffer<elem_type>::make(size_t capacity){
        return typename bounded_buffer<elem_type>::sptr(new bounded_buffer_impl<elem_type>(capacity));
    }

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_IPP */
