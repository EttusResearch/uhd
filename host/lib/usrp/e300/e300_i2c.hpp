//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_I2C_HPP
#define INCLUDED_E300_I2C_HPP

#include <boost/noncopyable.hpp>
#include <stdint.h>
#include <boost/shared_ptr.hpp>

#include <uhd/transport/zero_copy.hpp>

namespace uhd { namespace usrp { namespace e300 {

struct i2c_transaction_t {
    i2c_transaction_t(): reg(0), addr(0), data(0), type(0) {};
    uint16_t reg;
    uint8_t  addr;
    uint8_t  data;
    uint8_t  type;
};

class i2c : public boost::noncopyable
{
public:
    typedef boost::shared_ptr<i2c> sptr;

    static sptr make_i2cdev(const std::string &device);
    static sptr make_zc(uhd::transport::zero_copy_if::sptr xport);
    static sptr make_simple_udp(
        const std::string &ip_addr,
        const std::string &port);

    virtual uint8_t get_i2c_reg8(
        const uint8_t addr,
        const uint8_t reg) = 0;

    virtual uint8_t get_i2c_reg16(
        const uint8_t addr,
        const uint16_t reg) = 0;

    virtual void set_i2c_reg8(
        const uint8_t addr,
        const uint8_t reg,
        const uint8_t value) = 0;

    virtual void set_i2c_reg16(
        const uint8_t addr,
        const uint16_t reg,
        const uint8_t value) = 0;


    static const uint8_t DB_EEPROM_ADDR = 0x50;
    static const uint8_t MB_EEPROM_ADDR = 0x51;

    static const uint8_t WRITE          = 0x1;
    static const uint8_t READ           = 0x0;
    static const uint8_t TWOBYTE        = 0x4;
    static const uint8_t ONEBYTE        = 0x2;
};

}}};

#endif // INCLUDED_E300_I2C_HPP
