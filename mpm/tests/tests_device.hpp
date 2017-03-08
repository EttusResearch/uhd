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

#include <mpm/tests/tests_spi_iface.hpp>
#include <mpm/spi_iface.hpp>
#include <memory>

namespace mpm { namespace tests {
    class tests_dboard_periph_manager
    {
    public:
        typedef std::shared_ptr<tests_dboard_periph_manager> sptr;
        static sptr make();

    private:
        mpm::spi_iface::sptr _dev1_spi;
        mpm::spi_iface::sptr _dev2_spi;
    };
}
}
