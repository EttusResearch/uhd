//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_REF_VECTOR_HPP
#define INCLUDED_UHD_TYPES_REF_VECTOR_HPP

#include <uhd/config.hpp>

namespace uhd{

/*!
 * Reference vector:
 *  - Provides a std::vector-like interface for an array.
 *  - Statically sized, and does not manage the memory.
 */
template <typename T> class ref_vector{
public:
    //! Create a reference vector of length one from a pointer
    template <typename Ptr> ref_vector(Ptr *ptr):
        _mem(memp_t(&ptr)), _size(1)
    {
        /* NOP */
    }

    //! Create a reference vector from a std::vector container
    template <typename Range> ref_vector(const Range &range):
        _mem(memp_t(&range[0])), _size(range.size())
    {
        /* NOP */
    }

    //! Create a reference vector from a memory pointer and size
    ref_vector(T *mem, size_t size):
        _mem(mem), _size(size)
    {
        /* NOP */
    }

    T &operator[](size_t index) const{
        return _mem[index];
    }

    size_t size(void) const{
        return _size;
    }

private:
    typedef T* memp_t;
    const memp_t _mem;
    const size_t _size;
};

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_REF_VECTOR_HPP */
