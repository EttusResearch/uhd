//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/types/lockable.hpp>

using namespace mpm::types;

class lockable_impl : public lockable
{
public:
    lockable_impl(
            std::shared_ptr<std::mutex> spi_mutex
    ) : _spi_mutex(spi_mutex)
    {
        /* nop */
    }

    void lock()
    {
        _spi_mutex->lock();
    }

    void unlock()
    {
        _spi_mutex->unlock();
    }

private:
    std::shared_ptr<std::mutex> _spi_mutex;
};

lockable::sptr lockable::make(
            std::shared_ptr<std::mutex> spi_mutex
) {
    return std::make_shared<lockable_impl>(
        spi_mutex
    );
}

