//
// Copyright 2011-2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TRANSPORT_BUFFER_POOL_HPP
#define INCLUDED_UHD_TRANSPORT_BUFFER_POOL_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

    /*!
     * A buffer pool manages memory for a homogeneous set of buffers.
     * Each buffer is the pool start at a 16-byte alignment boundary.
     */
    class UHD_API buffer_pool : boost::noncopyable{
    public:
        typedef boost::shared_ptr<buffer_pool> sptr;
        typedef void * ptr_type;

        /*!
         * Make a new buffer pool.
         * \param num_buffs the number of buffers to allocate
         * \param buff_size the size of each buffer in bytes
         * \param alignment the alignment boundary in bytes
         * \return a new buffer pool buff_size X num_buffs
         */
        static sptr make(
            const size_t num_buffs,
            const size_t buff_size,
            const size_t alignment = 16
        );

        //! Get a pointer to the buffer start at the specified index
        virtual ptr_type at(const size_t index) const = 0;

        //! Get the number of buffers in this pool
        virtual size_t size(void) const = 0;
    };

}} //namespace


#endif /* INCLUDED_UHD_TRANSPORT_BUFFER_POOL_HPP */
