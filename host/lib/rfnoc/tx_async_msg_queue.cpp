//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/rfnoc/tx_async_msg_queue.hpp>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;

tx_async_msg_queue::tx_async_msg_queue(size_t capacity) : _queue(capacity) {}

bool tx_async_msg_queue::recv_async_msg(
    uhd::async_metadata_t& async_metadata, int32_t timeout_ms)
{
    using namespace std::chrono;

    if (timeout_ms == 0.0) {
        return _queue.pop(async_metadata);
    }

    const auto end_time = steady_clock::now() + milliseconds(timeout_ms);

    bool last_check = false;

    while (true) {
        if (_queue.pop(async_metadata)) {
            return true;
        }

        if (steady_clock::now() > end_time) {
            if (last_check) {
                return false;
            } else {
                last_check = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void tx_async_msg_queue::enqueue(const async_metadata_t& async_metadata)
{
    _queue.push(async_metadata);
}
