//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/transport/bounded_buffer.ipp> //detail

namespace uhd{ namespace transport{

    /*!
     * Implement a templated bounded buffer:
     * Used for passing elements between threads in a producer-consumer model.
     * The bounded buffer implemented waits and timed waits with condition variables.
     * The pop operation blocks on the bounded_buffer to become non empty.
     * The push operation blocks on the bounded_buffer to become non full.
     */
    template <typename elem_type> class bounded_buffer{
    public:

        /*!
         * Create a new bounded buffer object.
         * \param capacity the bounded_buffer capacity
         */
        bounded_buffer(size_t capacity):
            _detail(capacity)
        {
            /* NOP */
        }

        /*!
         * Push a new element into the bounded buffer immediately.
         * The element will not be pushed when the buffer is full.
         * \param elem the element reference pop to
         * \return false when the buffer is full
         */
        UHD_INLINE bool push_with_haste(const elem_type &elem){
            return _detail.push_with_haste(elem);
        }

        /*!
         * Push a new element into the bounded buffer.
         * If the buffer is full prior to the push,
         * make room by poping the oldest element.
         * \param elem the new element to push
         * \return true if the element fit without popping for space
         */
        UHD_INLINE bool push_with_pop_on_full(const elem_type &elem){
            return _detail.push_with_pop_on_full(elem);
        }

        /*!
         * Push a new element into the bounded_buffer.
         * Wait until the bounded_buffer becomes non-full.
         * \param elem the new element to push
         */
        UHD_INLINE void push_with_wait(const elem_type &elem){
            return _detail.push_with_wait(elem);
        }

        /*!
         * Push a new element into the bounded_buffer.
         * Wait until the bounded_buffer becomes non-full or timeout.
         * \param elem the new element to push
         * \param timeout the timeout in seconds
         * \return false when the operation times out
         */
        UHD_INLINE bool push_with_timed_wait(const elem_type &elem, double timeout){
            return _detail.push_with_timed_wait(elem, timeout);
        }

        /*!
         * Pop an element from the bounded buffer immediately.
         * The element will not be popped when the buffer is empty.
         * \param elem the element reference pop to
         * \return false when the buffer is empty
         */
        UHD_INLINE bool pop_with_haste(elem_type &elem){
            return _detail.pop_with_haste(elem);
        }

        /*!
         * Pop an element from the bounded_buffer.
         * Wait until the bounded_buffer becomes non-empty.
         * \param elem the element reference pop to
         */
        UHD_INLINE void pop_with_wait(elem_type &elem){
            return _detail.pop_with_wait(elem);
        }

        /*!
         * Pop an element from the bounded_buffer.
         * Wait until the bounded_buffer becomes non-empty or timeout.
         * \param elem the element reference pop to
         * \param timeout the timeout in seconds
         * \return false when the operation times out
         */
        UHD_INLINE bool pop_with_timed_wait(elem_type &elem, double timeout){
            return _detail.pop_with_timed_wait(elem, timeout);
        }

    private: bounded_buffer_detail<elem_type> _detail;
    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_BOUNDED_BUFFER_HPP */
