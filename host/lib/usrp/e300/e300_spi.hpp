//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_SPI_HPP
#define INCLUDED_E300_SPI_HPP

#include <uhd/types/serial.hpp>

namespace uhd { namespace usrp { namespace e300 {

class spi : public virtual uhd::spi_iface
{
public:
    typedef boost::shared_ptr<spi> sptr;
    static sptr make(const std::string &device);
};

}}};

#endif /* INCLUDED_E300_SPI_HPP */
