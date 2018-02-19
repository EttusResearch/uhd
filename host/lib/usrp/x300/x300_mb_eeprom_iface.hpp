//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_MB_EEPROM_HPP
#define INCLUDED_X300_MB_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class x300_mb_eeprom_iface : public uhd::i2c_iface
{
public:
    typedef boost::shared_ptr<x300_mb_eeprom_iface> sptr;

    virtual ~x300_mb_eeprom_iface(void) = 0;

    static sptr make(uhd::wb_iface::sptr wb, uhd::i2c_iface::sptr i2c);
};

#endif /* INCLUDED_X300_MB_EEPROM_HPP */
