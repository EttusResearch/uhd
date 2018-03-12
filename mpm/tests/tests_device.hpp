//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
