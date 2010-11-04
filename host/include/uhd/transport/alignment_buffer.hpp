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

#ifndef INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP
#define INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP

#include <uhd/config.hpp>
#include <uhd/transport/bounded_buffer.hpp> //time_duration_t
#include <boost/shared_ptr.hpp>
#include <vector>

namespace uhd{ namespace transport{

    /*!
     * Implement a templated alignment buffer:
     * Used for aligning asynchronously pushed elements with matching ids.
     */
    template <typename elem_type, typename seq_type> class alignment_buffer{
    public:
        typedef boost::shared_ptr<alignment_buffer<elem_type, seq_type> > sptr;

        /*!
         * Make a new alignment buffer object.
         * \param capacity the maximum elements per index
         * \param width the number of elements to align
         */
        static sptr make(size_t capacity, size_t width);

        /*!
         * Push an element with sequence id into the buffer at index.
         * \param elem the element to push
         * \param seq the sequence identifier
         * \param index the buffer index
         * \return true if the element fit without popping for space
         */
        virtual bool push_with_pop_on_full(
            const elem_type &elem, const seq_type &seq, size_t index
        ) = 0;

        /*!
         * Pop an aligned set of elements from this alignment buffer.
         * \param elems a collection to store the aligned elements
         * \param timeout the timeout in seconds
         * \return false when the operation times out
         */
        virtual bool pop_elems_with_timed_wait(
            std::vector<elem_type> &elems, double timeout
        ) = 0;
    };

}} //namespace

#include <uhd/transport/alignment_buffer.ipp>

#endif /* INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP */
