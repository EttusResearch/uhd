//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <cstddef>
#include <memory>
#include <vector>

namespace uhd { namespace transport {

/*!
 * A buffer pool manages memory for a homogeneous set of buffers.
 * Each buffer is the pool start at a 16-byte alignment boundary.
 */
class UHD_API buffer_pool : uhd::noncopyable
{
public:
    typedef std::shared_ptr<buffer_pool> sptr;
    typedef void* ptr_type;

    ~buffer_pool() = default;

    /*!
     * Make a new buffer pool.
     * \param num_buffs the number of buffers to allocate
     * \param buff_size the size of each buffer in bytes
     * \param alignment the alignment boundary in bytes
     * \return a new buffer pool buff_size X num_buffs
     */
    static sptr make(
        const size_t num_buffs, const size_t buff_size, const size_t alignment = 16);

    //! Get a pointer to the buffer start at the specified index
    ptr_type at(const size_t index) const
    {
        return _ptrs.at(index);
    }

    //! Get the number of buffers in this pool
    size_t size() const
    {
        return _ptrs.size();
    }

private:
    buffer_pool(std::vector<ptr_type>&& ptrs, std::unique_ptr<char[]> mem)
        : _ptrs(std::move(ptrs)), _mem(std::move(mem))
    {
    }

    std::vector<ptr_type> _ptrs;
    std::unique_ptr<char[]> _mem;
};

}} // namespace uhd::transport
