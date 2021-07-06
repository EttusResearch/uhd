//
// Copyright 2013,2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <stdint.h>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

namespace uhd { namespace usrp {

struct recv_packet_demuxer_3000 : std::enable_shared_from_this<recv_packet_demuxer_3000>
{
    typedef std::shared_ptr<recv_packet_demuxer_3000> sptr;
    static sptr make(transport::zero_copy_if::sptr xport)
    {
        return sptr(new recv_packet_demuxer_3000(xport));
    }

    recv_packet_demuxer_3000(transport::zero_copy_if::sptr xport) : _xport(xport)
    { /*NOP*/
    }

    transport::managed_recv_buffer::sptr get_recv_buff(
        const uint32_t sid, const double timeout)
    {
        const auto exit_time = std::chrono::high_resolution_clock::now()
                               + std::chrono::microseconds(int64_t(timeout * 1e6));
        transport::managed_recv_buffer::sptr buff;
        buff = _internal_get_recv_buff(sid, timeout);
        while (not buff) // loop until timeout
        {
            const auto delta = exit_time - std::chrono::high_resolution_clock::now();
            const double new_timeout =
                std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
            if (new_timeout < 0.0) {
                break;
            }
            buff = _internal_get_recv_buff(sid, new_timeout);
        }
        return buff;
    }

    transport::managed_recv_buffer::sptr _internal_get_recv_buff(
        const uint32_t sid, const double timeout)
    {
        transport::managed_recv_buffer::sptr buff;

        //----------------------------------------------------------
        //-- Check the queue to see if we already have a buffer
        //----------------------------------------------------------
        {
            std::lock_guard<std::mutex> l(mutex);
            queue_type_t& queue = _queues[sid];
            if (not queue.empty()) {
                buff = queue.front();
                queue.front().reset();
                queue.pop();
                return buff;
            }
        }
        {
            buff = _xport->get_recv_buff(timeout);
            if (buff) {
                const uint32_t new_sid = uhd::wtohx(buff->cast<const uint32_t*>()[1]);
                if (new_sid != sid) {
                    std::lock_guard<std::mutex> l(mutex);
                    if (_queues.count(new_sid) == 0)
                        UHD_LOGGER_ERROR("STREAMER")
                            << "recv packet demuxer unexpected sid 0x" << std::hex
                            << new_sid << std::dec;
                    else
                        _queues[new_sid].push(buff);
                    buff.reset();
                }
            }
        }
        return buff;
    }

    void realloc_sid(const uint32_t sid)
    {
        std::lock_guard<std::mutex> l(mutex);
        while (not _queues[sid].empty()) // allocated and clears if already allocated
        {
            _queues[sid].pop();
        }
    }

    transport::zero_copy_if::sptr make_proxy(const uint32_t sid);

    typedef std::queue<transport::managed_recv_buffer::sptr> queue_type_t;
    std::map<uint32_t, queue_type_t> _queues;
    transport::zero_copy_if::sptr _xport;
    std::mutex mutex;
};

struct recv_packet_demuxer_proxy_3000 : transport::zero_copy_if
{
    recv_packet_demuxer_proxy_3000(recv_packet_demuxer_3000::sptr demux,
        transport::zero_copy_if::sptr xport,
        const uint32_t sid)
        : _demux(demux), _xport(xport), _sid(sid)
    {
        _demux->realloc_sid(_sid); // causes clear
    }

    ~recv_packet_demuxer_proxy_3000(void) override
    {
        _demux->realloc_sid(_sid); // causes clear
    }

    size_t get_num_recv_frames(void) const override
    {
        return _xport->get_num_recv_frames();
    }
    size_t get_recv_frame_size(void) const override
    {
        return _xport->get_recv_frame_size();
    }
    transport::managed_recv_buffer::sptr get_recv_buff(double timeout) override
    {
        return _demux->get_recv_buff(_sid, timeout);
    }
    size_t get_num_send_frames(void) const override
    {
        return _xport->get_num_send_frames();
    }
    size_t get_send_frame_size(void) const override
    {
        return _xport->get_send_frame_size();
    }
    transport::managed_send_buffer::sptr get_send_buff(double timeout) override
    {
        return _xport->get_send_buff(timeout);
    }

    recv_packet_demuxer_3000::sptr _demux;
    transport::zero_copy_if::sptr _xport;
    const uint32_t _sid;
};

inline transport::zero_copy_if::sptr recv_packet_demuxer_3000::make_proxy(
    const uint32_t sid)
{
    return transport::zero_copy_if::sptr(
        new recv_packet_demuxer_proxy_3000(this->shared_from_this(), _xport, sid));
}

}} // namespace uhd::usrp
