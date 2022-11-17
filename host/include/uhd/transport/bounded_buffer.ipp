//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/condition.hpp>
#include <boost/utility.hpp>
#include <functional>
#include <mutex>

namespace uhd { namespace transport {

template <typename elem_type>
class bounded_buffer_detail : uhd::noncopyable
{
public:
    bounded_buffer_detail(size_t capacity) : _buffer(capacity)
    {
        _not_full_fcn  = std::bind(&bounded_buffer_detail<elem_type>::not_full, this);
        _not_empty_fcn = std::bind(&bounded_buffer_detail<elem_type>::not_empty, this);
    }

    UHD_INLINE bool push_with_haste(const elem_type& elem)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.full()) {
            return false;
        }
        _buffer.push_front(elem);
        _empty_cond.notify_one();
        return true;
    }

    UHD_INLINE bool push_with_pop_on_full(const elem_type& elem)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_buffer.full()) {
            _buffer.pop_back();
            _buffer.push_front(elem);
            _empty_cond.notify_one();
            return false;
        } else {
            _buffer.push_front(elem);
            _empty_cond.notify_one();
            return true;
        }
    }

    UHD_INLINE void push_with_wait(const elem_type& elem)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.full()) {
            _full_cond.wait(lock, _not_full_fcn);
        }
        _buffer.push_front(elem);
        _empty_cond.notify_one();
    }

    UHD_INLINE bool push_with_timed_wait(const elem_type& elem, double timeout)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.full()) {
            if (not _full_cond.timed_wait(lock, to_time_dur(timeout), _not_full_fcn)) {
                return false;
            }
        }
        _buffer.push_front(elem);
        _empty_cond.notify_one();
        return true;
    }

    UHD_INLINE bool pop_with_haste(elem_type& elem)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.empty()) {
            return false;
        }
        this->pop_back(elem);
        _full_cond.notify_one();
        return true;
    }

    UHD_INLINE void pop_with_wait(elem_type& elem)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.empty()) {
            _empty_cond.wait(lock, _not_empty_fcn);
        }
        this->pop_back(elem);
        _full_cond.notify_one();
    }

    UHD_INLINE bool pop_with_timed_wait(elem_type& elem, double timeout)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_buffer.empty()) {
            if (not _empty_cond.timed_wait(lock, to_time_dur(timeout), _not_empty_fcn)) {
                return false;
            }
        }
        this->pop_back(elem);
        _full_cond.notify_one();
        return true;
    }

private:
    std::mutex _mutex;
    boost::condition _empty_cond, _full_cond;
    boost::circular_buffer<elem_type> _buffer;

    bool not_full(void) const
    {
        return not _buffer.full();
    }
    bool not_empty(void) const
    {
        return not _buffer.empty();
    }

    std::function<bool(void)> _not_full_fcn, _not_empty_fcn;

    /*!
     * Three part operation to pop an element:
     * 1) assign elem to the back element
     * 2) assign the back element to empty
     * 3) pop the back to move the counter
     */
    UHD_INLINE void pop_back(elem_type& elem)
    {
        elem           = _buffer.back();
        _buffer.back() = elem_type();
        _buffer.pop_back();
    }

    static UHD_INLINE boost::posix_time::time_duration to_time_dur(double timeout)
    {
        return boost::posix_time::microseconds(long(timeout * 1e6));
    }
};
}} // namespace uhd::transport
