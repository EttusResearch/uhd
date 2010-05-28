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

#ifndef INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_HPP
#define INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_HPP

#include <uhd/config.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/condition.hpp>

namespace uhd{ namespace transport{

    /*!
     * Imlement a templated bounded buffer:
     * Used for passing elements between threads in a producer-consumer model.
     * The bounded buffer implemented waits and timed waits with condition variables.
     * The pop operation blocks on the bounded_buffer to become non empty.
     * The push operation blocks on the bounded_buffer to become non full.
     */
    template <typename elem_type> class bounded_buffer{
    public:
        typedef boost::shared_ptr<bounded_buffer<elem_type> > sptr;

        /*!
         * Create a new bounded_buffer of a given size.
         * \param capacity the bounded_buffer capacity
         */
        bounded_buffer(size_t capacity) : _buffer(capacity){
            /* NOP */
        }

        /*!
         * Destroy this bounded_buffer.
         */
        ~bounded_buffer(void){
            /* NOP */
        }

        /*!
         * Push a new element into the bounded buffer.
         * If the buffer is full prior to the push,
         * make room by poping the oldest element.
         * \param elem the new element to push
         * \return true if the element fit without popping for space
         */
        UHD_INLINE bool push_with_pop_on_full(const elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if(_buffer.full()){
                _buffer.pop_back();
                _buffer.push_front(elem);
                return false;
            }
            else{
                _buffer.push_front(elem);
                return true;
            }
        }

        /*!
         * Push a new element into the bounded_buffer.
         * Wait until the bounded_buffer becomes non-full.
         * \param elem the new element to push
         */
        UHD_INLINE void push_with_wait(const elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            _full_cond.wait(lock, boost::bind(&bounded_buffer<elem_type>::not_full, this));
            _buffer.push_front(elem);
            lock.unlock();
            _empty_cond.notify_one();
        }

        /*!
         * Push a new element into the bounded_buffer.
         * Wait until the bounded_buffer becomes non-full or timeout.
         * \param elem the new element to push
         * \param time the timeout time
         * \return false when the operation times out
         */
        template<typename time_type> UHD_INLINE
        bool push_with_timed_wait(const elem_type &elem, const time_type &time){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if (not _full_cond.timed_wait(lock, time, boost::bind(&bounded_buffer<elem_type>::not_full, this))) return false;
            _buffer.push_front(elem);
            lock.unlock();
            _empty_cond.notify_one();
            return true;
        }

        /*!
         * Pop an element from the bounded_buffer.
         * Wait until the bounded_buffer becomes non-empty.
         * \param elem the element reference pop to
         */
        UHD_INLINE void pop_with_wait(elem_type &elem){
            boost::unique_lock<boost::mutex> lock(_mutex);
            _empty_cond.wait(lock, boost::bind(&bounded_buffer<elem_type>::not_empty, this));
            elem = _buffer.back(); _buffer.pop_back();
            lock.unlock();
            _full_cond.notify_one();
        }

        /*!
         * Pop an element from the bounded_buffer.
         * Wait until the bounded_buffer becomes non-empty or timeout.
         * \param elem the element reference pop to
         * \param time the timeout time
         * \return false when the operation times out
         */
        template<typename time_type> UHD_INLINE
        bool pop_with_timed_wait(elem_type &elem, const time_type &time){
            boost::unique_lock<boost::mutex> lock(_mutex);
            if (not _empty_cond.timed_wait(lock, time, boost::bind(&bounded_buffer<elem_type>::not_empty, this))) return false;
            elem = _buffer.back(); _buffer.pop_back();
            lock.unlock();
            _full_cond.notify_one();
            return true;
        }

    private:
        boost::mutex _mutex;
        boost::condition _empty_cond, _full_cond;
        boost::circular_buffer<elem_type> _buffer;

        bool not_full(void) const{return not _buffer.full();}
        bool not_empty(void) const{return not _buffer.empty();}
    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_HPP */
