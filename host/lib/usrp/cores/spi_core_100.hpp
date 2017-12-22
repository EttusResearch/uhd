//
// Copyright 2011,2014 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_USRP_SPI_CORE_100_HPP
#define INCLUDED_LIBUHD_USRP_SPI_CORE_100_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class spi_core_100 : boost::noncopyable, public uhd::spi_iface{
public:
    typedef boost::shared_ptr<spi_core_100> sptr;

    virtual ~spi_core_100(void) = 0;

    //! makes a new spi core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base);
};

#endif /* INCLUDED_LIBUHD_USRP_SPI_CORE_100_HPP */
