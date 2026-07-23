/*
 * Copyright (c) 2013 National Instruments
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#pragma once

#include "NiFpga.h"
#include <chrono> // std::chrono::

namespace nirio {

/**
 * Timer for measuring elapsed times and whether given timeouts have occurred.
 * For compatibility with NiFpga.h, timeouts are uint32_ts, all times are in
 * milliseconds, and NiFpga_InfiniteTimeout is used for infinite timeouts.
 */
class Timer
{
public:
    explicit Timer(const uint32_t timeout = NiFpga_InfiniteTimeout)
        : // start set below in reset()
        timeout(timeout)
    {
        reset();
    }

    void reset()
    {
        start = Clock::now();
    }

    uint32_t getElapsed() const
    {
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start)
                .count());
    }

    uint32_t getRemaining() const
    {
        // infinite timeout always has infinite remaining
        if (isInfinite())
            return NiFpga_InfiniteTimeout;
        // see how long it's been
        const auto elapsed = getElapsed();
        // if it's been long enough, nothing's remaining
        if (elapsed >= timeout)
            return 0;
        // otherwise, calculate what's left
        else
            return timeout - elapsed;
    }

    bool isTimedOut() const
    {
        return getRemaining() == 0;
    }

    bool isInfinite() const
    {
        return timeout == NiFpga_InfiniteTimeout;
    }

private:
    typedef std::chrono::high_resolution_clock Clock;

    Clock::time_point start;
    const uint32_t timeout;
};

} // namespace nirio
