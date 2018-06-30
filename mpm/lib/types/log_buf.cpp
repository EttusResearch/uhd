//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/types/log_buf.hpp>

using namespace mpm::types;

void log_buf::post(
    const log_level_t log_level,
    const std::string &component,
    const std::string &message
) {
    {
        std::lock_guard<std::mutex> l(_buf_lock);
        _buf.push_back(
            log_message(log_level, component, message)
        );
    }

    if (bool(_notify_callback)) {
        _notify_callback();
    }
}

void log_buf::set_notify_callback(
    std::function<void(void)> callback
) {
    _notify_callback = callback;
}

std::tuple<log_level_t, std::string, std::string> log_buf::pop()
{
    std::lock_guard<std::mutex> l(_buf_lock);
    if (_buf.empty()) {
        return std::make_tuple(
            log_level_t::NONE,
            "",
            ""
        );
    }

    auto last_msg = _buf.front();
    _buf.pop_front();
    return std::make_tuple(
        last_msg.log_level,
        last_msg.component,
        last_msg.message
    );
}

log_buf::sptr log_buf::make()
{
    return std::make_shared<log_buf>();
}

log_buf::sptr log_buf::make_singleton()
{
    static auto log_sptr = log_buf::make();
    return log_sptr;
}

