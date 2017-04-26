//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

