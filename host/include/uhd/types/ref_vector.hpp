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
    //! Create a reference vector from a pointer and size
    template <typename Ptr> ref_vector(Ptr *ptr, size_t size = 1):
        _mem(T(ptr)), _size(size)
    {
        /* NOP */
    }

    //! Create a reference vector from a std::vector container
    template <typename Range> ref_vector(const Range &range):
        _mem(T(range.front())), _size(range.size())
    {
        /* NOP */
    }

    const T &operator[](size_t index) const{
        return (&_mem)[index];
    }

    size_t size(void) const{
        return _size;
    }

private:
    const T      _mem;
    const size_t _size;
};

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_REF_VECTOR_HPP */
