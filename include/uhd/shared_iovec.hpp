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

#ifndef INCLUDED_UHD_SHARED_IOVEC_HPP
#define INCLUDED_UHD_SHARED_IOVEC_HPP

#include <boost/shared_array.hpp>
#include <stdint.h>

namespace uhd{

/*!
 * A shared iovec contains a shared array and its length.
 * Creating a new shared iovec allocates new memory.
 * This memory is freed when all copies are destroyed.
 */
class shared_iovec{
public:
    /*!
     * Create a shared iovec and allocate memory.
     * \param len the length in bytes
     */
    shared_iovec(size_t len=0);

    /*!
     * Destroy a shared iovec.
     * Will not free the memory unless this is the last copy.
     */
    ~shared_iovec(void);

    void *base;
    size_t len;

private:
    boost::shared_array<uint8_t> _shared_array;
};

} //namespace uhd

#endif /* INCLUDED_UHD_SHARED_IOVEC_HPP */
