//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/buffer_pool.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <memory>
#include <vector>

using namespace uhd::transport;

//! pad the byte count to a multiple of alignment
static size_t pad_to_boundary(const size_t bytes, const size_t alignment)
{
    return bytes + (alignment - bytes) % alignment;
}

/***********************************************************************
 * Buffer pool factory function
 **********************************************************************/
buffer_pool::sptr buffer_pool::make(
    const size_t num_buffs, const size_t buff_size, const size_t alignment)
{
    // 1) pad the buffer size to be a multiple of alignment
    // 2) pad the overall memory size for room after alignment
    // 3) allocate the memory in one block of sufficient size
    const size_t padded_buff_size = pad_to_boundary(buff_size, alignment);
    std::unique_ptr<char[]> mem(new char[padded_buff_size * num_buffs + alignment - 1]);

    // Fill a vector with boundary-aligned points in the memory
    const size_t mem_start = pad_to_boundary(size_t(mem.get()), alignment);
    std::vector<ptr_type> ptrs(num_buffs);
    for (size_t i = 0; i < num_buffs; i++) {
        ptrs[i] = ptr_type(mem_start + padded_buff_size * i);
    }

    // Create a new buffer pool with:
    // - the pre-computed pointers, and
    // - the reference to allocated memory.
    return sptr(new buffer_pool(std::move(ptrs), std::move(mem)));
}
