//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/link_if.hpp>
#include <cassert>
#include <vector>

namespace uhd { namespace transport {
namespace detail {

/*!
 * Container for free buffers used by link base classes.
 */
class free_buff_pool
{
public:
    free_buff_pool(const size_t capacity)
    {
        _buffs.reserve(capacity);
    }

    frame_buff* pop()
    {
        // Free buffer pool should not be empty unless user has requested more
        // buffers than the number of frames in the link.
        assert(!_buffs.empty());

        frame_buff* buff = _buffs.back();
        _buffs.pop_back();
        return buff;
    }

    void push(frame_buff* buff)
    {
        _buffs.push_back(buff);
    }

private:
    std::vector<frame_buff*> _buffs;
};

} // namespace detail

/*!
 * Reusable base class implementation of send_link_iface. Link implementations
 * should derive from this template and pass their own type as the derived_t
 * template parameter.
 *
 * The base class template manages a pool of frame_buff pointers and implements
 * the link interface methods.
 *
 * This template requires the following methods in the derived class:
 *   bool get_send_buff_derived(frame_buff& buf, int32_t timeout_ms);
 *   void release_send_buf_derived(frame_buff& buf);
 *
 * Additionally, the subclass must call preload_free_buf for each frame_buff
 * object it owns during initialization to add it to the free buffer pool.
 *
 * \param derived_t type of the derived class
 */
template <typename derived_t>
class send_link_base : public virtual send_link_if
{
public:
    send_link_base(const size_t num_frames, const size_t frame_size)
        : _send_frame_size(frame_size)
        , _num_send_frames(num_frames)
        , _free_send_buffs(num_frames)
    {
    }

    virtual ~send_link_base() = default;

    size_t get_num_send_frames() const override
    {
        return _num_send_frames;
    }

    size_t get_send_frame_size() const override
    {
        return _send_frame_size;
    }

    frame_buff::uptr get_send_buff(int32_t timeout_ms) override
    {
        frame_buff* buff = _free_send_buffs.pop();

        // Call the derived class for link-specific implementation
        auto* derived = static_cast<derived_t*>(this);

        if (!derived->get_send_buff_derived(*buff, timeout_ms)) {
            _free_send_buffs.push(buff);
            return frame_buff::uptr();
        }

        return frame_buff::uptr(buff);
    }

    void release_send_buff(frame_buff::uptr buff) override
    {
        frame_buff* buff_ptr = buff.release();
        assert(buff_ptr);

        if (buff_ptr->packet_size() != 0) {
            // Call the derived class for link-specific implementation
            auto* derived = static_cast<derived_t*>(this);
            derived->release_send_buff_derived(*buff_ptr);
        }

        // Reset buff and re-add to free pool
        buff_ptr->set_packet_size(0);
        _free_send_buffs.push(buff_ptr);
    }

protected:
    /*!
     * Add buffer pointer to free buffer pool.
     *
     * Derived classes should call this method during initialization for each
     * frame buffer it owns.
     *
     * \param buffer pointer to the buffer to add to the free buffer pool.
     */
    void preload_free_buff(frame_buff* buff)
    {
        _free_send_buffs.push(buff);
    }

private:
    size_t _send_frame_size;
    size_t _num_send_frames;
    detail::free_buff_pool _free_send_buffs;
};

/*!
 * Reusable base class implementation of recv_link_if. Link implementations
 * should derive from this template and pass their own type as the derived_t
 * template parameter.
 *
 * The base class template manages a pool of free_buff pointers and implements
 * the link interface methods.
 *
 * This template requires the following methods in the derived class:
 *   size_t get_recv_buff_derived(frame_buff& buff, int32_t timeout_ms);
 *   void release_recv_buff_derived(frame_buff& buff);
 *
 * Additionally, the subclass must call preload_free_buff for each
 * frame_buff object it owns during initialization to add it to the free
 * buff pool.
 *
 * \param derived_t type of the derived class
 */
template <typename derived_t>
class recv_link_base : public virtual recv_link_if
{
public:
    recv_link_base(const size_t num_frames, const size_t frame_size)
        : _recv_frame_size(frame_size)
        , _num_recv_frames(num_frames)
        , _free_recv_buffs(num_frames)
    {
    }

    virtual ~recv_link_base() = default;

    size_t get_num_recv_frames() const override
    {
        return _num_recv_frames;
    }

    size_t get_recv_frame_size() const override
    {
        return _recv_frame_size;
    }

    frame_buff::uptr get_recv_buff(int32_t timeout_ms) override
    {
        frame_buff* buff = _free_recv_buffs.pop();

        // Call the derived class for link specific implementation
        auto* derived = static_cast<derived_t*>(this);

        size_t len = derived->get_recv_buff_derived(*buff, timeout_ms);

        if (len == 0) {
            _free_recv_buffs.push(buff);
            return frame_buff::uptr();
        } else {
            buff->set_packet_size(len);
            return frame_buff::uptr(buff);
        }
    }

    void release_recv_buff(frame_buff::uptr buff) override
    {
        frame_buff* buff_ptr = buff.release();
        assert(buff_ptr);

        // Call the derived class for link specific implementation
        auto* derived = static_cast<derived_t*>(this);

        derived->release_recv_buff_derived(*buff_ptr);

        // Reset buffer and re-add to free pool
        buff_ptr->set_packet_size(0);
        _free_recv_buffs.push(buff_ptr);
    }

protected:
    /*!
     * Add buffer pointer to free buffer pool.
     *
     * Derived classes should call this method during initialization for each
     * frame buffer it owns.
     *
     * \param buffer pointer to the buffer to add to the free buffer pool.
     */
    void preload_free_buff(frame_buff* buff)
    {
        _free_recv_buffs.push(buff);
    }

private:
    size_t _recv_frame_size;
    size_t _num_recv_frames;
    detail::free_buff_pool _free_recv_buffs;
};

}} // namespace uhd::transport
