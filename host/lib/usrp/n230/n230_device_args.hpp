//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_DEV_ARGS_HPP
#define INCLUDED_N230_DEV_ARGS_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhdlib/usrp/constrained_device_args.hpp>
#include "n230_defaults.h"
#include <boost/thread/mutex.hpp>

namespace uhd { namespace usrp { namespace n230 {

class n230_device_args_t : public constrained_device_args_t
{
public:
    enum loopback_mode_t { LOOPBACK_OFF=0, LOOPBACK_RADIO=1, LOOPBACK_CODEC=2 };

    n230_device_args_t():
        _master_clock_rate("master_clock_rate", n230::DEFAULT_TICK_RATE),
        _send_frame_size("send_frame_size", n230::DEFAULT_FRAME_SIZE),
        _recv_frame_size("recv_frame_size", n230::DEFAULT_FRAME_SIZE),
        _num_send_frames("num_send_frames", n230::DEFAULT_NUM_FRAMES),
        _num_recv_frames("num_recv_frames", n230::DEFAULT_NUM_FRAMES),
        _send_buff_size("send_buff_size", n230::DEFAULT_SEND_BUFF_SIZE),
        _recv_buff_size("recv_buff_size", n230::DEFAULT_RECV_BUFF_SIZE),
        _safe_mode("safe_mode", false),
        _loopback_mode("loopback_mode",
                       LOOPBACK_OFF,
                       {{"off", LOOPBACK_OFF}, {"radio", LOOPBACK_RADIO}, {"codec", LOOPBACK_CODEC}})
    {}

    double get_master_clock_rate() const {
        return _master_clock_rate.get();
    }
    size_t get_send_frame_size() const {
        return _send_frame_size.get();
    }
    size_t get_recv_frame_size() const {
        return _recv_frame_size.get();
    }
    size_t get_num_send_frames() const {
        return _num_send_frames.get();
    }
    size_t get_num_recv_frames() const {
        return _num_recv_frames.get();
    }
    size_t get_send_buff_size() const {
        return _send_buff_size.get();
    }
    size_t get_recv_buff_size() const {
        return _recv_buff_size.get();
    }
    bool get_safe_mode() const {
        return _safe_mode.get();
    }
    loopback_mode_t get_loopback_mode() const {
        return _loopback_mode.get();
    }

    inline virtual std::string to_string() const {
        return  _master_clock_rate.to_string() + ", " +
                _send_frame_size.to_string() + ", " +
                _recv_frame_size.to_string() + ", " +
                _num_send_frames.to_string() + ", " +
                _num_recv_frames.to_string() + ", " +
                _send_buff_size.to_string() + ", " +
                _recv_buff_size.to_string() + ", " +
                _safe_mode.to_string() + ", " +
                _loopback_mode.to_string();
    }
private:
    virtual void _parse(const device_addr_t& dev_args) {
        //Extract parameters from dev_args
        if (dev_args.has_key(_master_clock_rate.key()))
            _master_clock_rate.parse(dev_args[_master_clock_rate.key()]);
        if (dev_args.has_key(_send_frame_size.key()))
            _send_frame_size.parse(dev_args[_send_frame_size.key()]);
        if (dev_args.has_key(_recv_frame_size.key()))
            _recv_frame_size.parse(dev_args[_recv_frame_size.key()]);
        if (dev_args.has_key(_num_send_frames.key()))
            _num_send_frames.parse(dev_args[_num_send_frames.key()]);
        if (dev_args.has_key(_num_recv_frames.key()))
            _num_recv_frames.parse(dev_args[_num_recv_frames.key()]);
        if (dev_args.has_key(_send_buff_size.key()))
            _send_buff_size.parse(dev_args[_send_buff_size.key()]);
        if (dev_args.has_key(_recv_buff_size.key()))
            _recv_buff_size.parse(dev_args[_recv_buff_size.key()]);
        if (dev_args.has_key(_safe_mode.key()))
            _safe_mode.parse(dev_args[_safe_mode.key()]);
        if (dev_args.has_key(_loopback_mode.key()))
            _loopback_mode.parse(dev_args[_loopback_mode.key()], false /* assert invalid */);

        //Sanity check params
        _enforce_range(_master_clock_rate, MIN_TICK_RATE, MAX_TICK_RATE);
        _enforce_range(_send_frame_size, MIN_FRAME_SIZE, MAX_FRAME_SIZE);
        _enforce_range(_recv_frame_size, MIN_FRAME_SIZE, MAX_FRAME_SIZE);
        _enforce_range(_num_send_frames, (size_t)2, (size_t)UINT_MAX);
        _enforce_range(_num_recv_frames, (size_t)2, (size_t)UINT_MAX);
    }

    constrained_device_args_t::num_arg<double>           _master_clock_rate;
    constrained_device_args_t::num_arg<size_t>           _send_frame_size;
    constrained_device_args_t::num_arg<size_t>           _recv_frame_size;
    constrained_device_args_t::num_arg<size_t>           _num_send_frames;
    constrained_device_args_t::num_arg<size_t>           _num_recv_frames;
    constrained_device_args_t::num_arg<size_t>           _send_buff_size;
    constrained_device_args_t::num_arg<size_t>           _recv_buff_size;
    constrained_device_args_t::bool_arg                  _safe_mode;
    constrained_device_args_t::enum_arg<loopback_mode_t> _loopback_mode;
};

}}} //namespace

#endif //INCLUDED_N230_DEV_ARGS_HPP
