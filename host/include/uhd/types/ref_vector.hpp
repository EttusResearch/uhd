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
template <typename T> class UHD_API ref_vector{
public:
    /*!
     * Create a reference vector of size 1 from a pointer.
     * Therefore: rv[0] == ptr and rv.size() == 1
     * \param ptr a pointer to a chunk of memory
     */
    template <typename Ptr> ref_vector(Ptr *ptr):
        _ptr(T(ptr)), _mem(_mem_t(&_ptr)), _size(1)
    {
        /* NOP */
    }

    /*!
     * Create a reference vector from a std::vector container.
     * Therefore: rv[n] == vec[n] and rv.size() == vec.size()
     * \param vec a const reference to an std::vector
     */
    template <typename Vector> ref_vector(const Vector &vec):
        _ptr(T()), _mem(_mem_t(&vec.front())), _size(vec.size())
    {
        /* NOP */
    }

    /*!
     * Create a reference vector from a pointer and a length
     * Therefore: rv[n] == mem[n] and rv.size() == len
     * \param mem a pointer to an array of pointers
     * \param len the length of the array of pointers
     */
    ref_vector(const T *mem, size_t len):
        _ptr(T()), _mem(_mem_t(mem)), _size(len)
    {
        /* NOP */
    }

    //! Index operator gets the value of rv[index]
    const T &operator[](size_t index) const{
        return _mem[index];
    }

    //! The number of elements in this container
    size_t size(void) const{
        return _size;
    }

private:
    const T      _ptr;
    typedef T*   _mem_t;
    const _mem_t _mem;
    const size_t _size;
};

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_REF_VECTOR_HPP */
