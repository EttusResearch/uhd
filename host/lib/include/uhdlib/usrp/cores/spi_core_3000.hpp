//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class spi_core_3000 : boost::noncopyable, public uhd::spi_iface
{
public:
    typedef boost::shared_ptr<spi_core_3000> sptr;

    virtual ~spi_core_3000(void) = 0;

    //! makes a new spi core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);

    //! Set the spi clock divider to something usable
    virtual void set_divider(const double div) = 0;

    //! Place SPI core in shutdown mode. All attempted SPI transactions are dropped by
    //  the core.
    virtual void set_shutdown(const bool shutdown) = 0;

    //! Get state of shutdown register
    virtual bool get_shutdown() = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP */
