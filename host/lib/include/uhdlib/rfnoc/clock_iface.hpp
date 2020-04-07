//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <atomic>
#include <memory>
#include <string>

namespace uhd { namespace rfnoc {

class clock_iface
{
public:
    using sptr = std::shared_ptr<clock_iface>;

    clock_iface(const std::string& name) : _name(name), _is_mutable(true)
    {
        _is_running = false;
        _freq       = 0.0;
    }

    clock_iface(const std::string& name, const double freq, const bool is_mutable = true)
        : _name(name), _freq(freq), _is_mutable(is_mutable)
    {
        _is_running = false;
    }

    clock_iface()                       = delete;
    clock_iface(const clock_iface& rhs) = delete;
    clock_iface(clock_iface&& rhs)      = delete;

    clock_iface& operator=(const clock_iface& fraction) = delete;

    inline const std::string& get_name() const
    {
        return _name;
    }

    inline bool is_running() const
    {
        return _is_running;
    }

    inline bool is_mutable() const
    {
        return _is_mutable;
    }

    inline void set_running(bool is_running)
    {
        _is_running = is_running;
    }

    inline double get_freq() const
    {
        return _freq;
    }

    //! If the clock is immutable, this will throw if freq is different from the
    // current frequency.
    inline void set_freq(double freq)
    {
        if (!_is_mutable && freq != _freq.load()) {
            UHD_LOG_ERROR(_name, "Trying to change an immutable clock!");
            throw uhd::runtime_error("Trying to change an immutable clock!");
        }
        _freq = freq;
    }

private:
    const std::string _name;
    std::atomic<bool> _is_running;
    std::atomic<double> _freq;
    const bool _is_mutable;
};

}} // namespace uhd::rfnoc
