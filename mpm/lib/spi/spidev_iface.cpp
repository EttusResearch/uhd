//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <mpm/spi/spi_iface.hpp>
#include <mpm/exception.hpp>
extern "C" {
#include "spidev.h"
}
#include <fcntl.h>
#include <linux/spi/spidev.h>

#include <boost/format.hpp>
#include <iostream>

using namespace mpm::spi;

/******************************************************************************
 * Implementation
 *****************************************************************************/
class spidev_iface_impl : public spi_iface
{
public:

    spidev_iface_impl(
            const std::string &device,
            const int max_speed_hz,
            const int spi_mode
    ) : _speed(max_speed_hz),
        _mode(spi_mode)
    {
        if (init_spi(
                &_fd,
                device.c_str(),
                _mode, _speed, _bits, _delay) < 0)
        {
            throw mpm::runtime_error(str(
                boost::format("Could not initialize spidev device %s")
                % device));
        }

        if (_fd < 0)
        {
            throw mpm::runtime_error(str(
                boost::format("Could not open spidev device %s")
                % device));
        }
    }

    ~spidev_iface_impl()
    {
        close(_fd);
    }

    uint32_t transfer24_8(
            const uint32_t data_
    ) {
        int ret(0);

        uint32_t data = data_;
        uint8_t *tx_data = reinterpret_cast<uint8_t *>(&data);

        // Create tx and rx buffers:
        uint8_t tx[] = {tx_data[2], tx_data[1], tx_data[0]}; // FIXME guarantee endianness
        uint8_t rx[3]; // Buffer length must match tx buffer

        if (transfer(
            _fd,
            &tx[0], &rx[0],
            3,
            _speed, _bits, _delay
        ) != 0) {
            throw mpm::runtime_error(str(
                    boost::format("SPI Transaction failed!")
            ));
        }

        return uint32_t(rx[2]);
    }

    uint32_t transfer24_16(
            const uint32_t data_
    ) {
        int ret(0);

        uint32_t data = data_;
        uint8_t *tx_data = reinterpret_cast<uint8_t *>(&data);

        // Create tx and rx buffers:
        uint8_t tx[] = {tx_data[2], tx_data[1], tx_data[0]}; // FIXME guarantee endianness
        uint8_t rx[3]; // Buffer length must match tx buffer

        if (transfer(
            _fd,
            &tx[0], &rx[0],
            3,
            _speed, _bits, _delay
        ) != 0) {
            throw mpm::runtime_error(str(
                    boost::format("SPI Transaction failed!")
            ));
        }

        return uint32_t(rx[1] << 8 | rx[2]);
    }

private:
    int _fd;
    const uint32_t _mode;
    uint32_t _speed = 2000000;
    uint8_t _bits = 8;
    uint16_t _delay = 0;
};

/******************************************************************************
 * Factory
 *****************************************************************************/
spi_iface::sptr spi_iface::make_spidev(
    const std::string &device,
    const int speed_hz,
    const int spi_mode
) {
    return std::make_shared<spidev_iface_impl>(
        device, speed_hz, spi_mode
    );
}

