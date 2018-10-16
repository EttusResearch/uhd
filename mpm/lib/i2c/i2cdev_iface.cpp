//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <mpm/i2c/i2c_iface.hpp>
#include <mpm/exception.hpp>

#include "i2cdev.h"

#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <boost/format.hpp>
#include <iostream>

using namespace mpm::i2c;

/******************************************************************************
 * Implementation
 *****************************************************************************/
class i2cdev_iface_impl : public i2c_iface
{
public:

    i2cdev_iface_impl(
            const std::string &device,
            const uint16_t addr,
            const bool ten_bit_addr,
            const unsigned int timeout_ms
    ) : _addr(addr),
        _ten_bit_addr(ten_bit_addr),
        _timeout_ms(timeout_ms)
    {
        if (i2cdev_open(
                &_fd,
                device.c_str(),
                timeout_ms) < 0)
        {
            throw mpm::runtime_error(str(
                boost::format("Could not initialize i2cdev device %s")
                % device));
        }

        if (_fd < 0)
        {
            throw mpm::runtime_error(str(
                boost::format("Could not open i2cdev device %s")
                % device));
        }
    }

    ~i2cdev_iface_impl()
    {
        close(_fd);
    }

    int transfer(uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
    {
        int ret = i2cdev_transfer(_fd, _addr, _ten_bit_addr,
                                  tx, tx_len, rx, rx_len);

        if (ret) {
            throw mpm::runtime_error(str(
                    boost::format("I2C Transaction failed!")
            ));
        }

        return ret;
    }

    int transfer(std::vector<uint8_t> *tx, std::vector<uint8_t> *rx)
    {
        uint8_t *tx_data = NULL, *rx_data = NULL;
        size_t tx_len = 0, rx_len = 0;

        if (tx) {
            tx_data = tx->data();
            tx_len = tx->size();
        }

        if (rx) {
            rx_data = rx->data();
            rx_len = rx->size();
        }
        int ret = transfer(tx_data, tx_len, rx_data, rx_len);
        return ret;
    }

private:
    int _fd;
    const uint16_t _addr;
    const bool _ten_bit_addr;
    const unsigned int _timeout_ms;
};

/******************************************************************************
 * Factory
 *****************************************************************************/
i2c_iface::sptr i2c_iface::make_i2cdev(
    const std::string &bus,
    const uint16_t addr,
    const bool ten_bit_addr,
    const int timeout_ms
) {
    return std::make_shared<i2cdev_iface_impl>(
        bus, addr, ten_bit_addr, timeout_ms
    );
}

