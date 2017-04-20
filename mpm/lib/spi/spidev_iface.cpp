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

#include "mpm/spi/spidev_iface.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <boost/format.hpp>

#include <iostream>

using namespace mpm::spi;

/******************************************************************************
 * Implementation
 *****************************************************************************/
class spidev_iface_impl : public spidev_iface
{
public:

    spidev_iface_impl(
            const std::string &device
    ) {
        int ret;

        _fd = open(device.c_str(), O_RDWR);
        if (_fd < 0) {
            throw std::runtime_error(str(
                    boost::format("Could not open spidev device %s")
                    % device
            ));
        }

        int MODE = 3;
        ret = ioctl(_fd, SPI_IOC_WR_MODE32, &MODE);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not set spidev mode to %X for spidev %s")
                    % uint16_t(_mode) % device
            ));
        }

        ret = ioctl(_fd, SPI_IOC_RD_MODE32, &_mode);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not get spidev mode for spidev %s")
                    % device
            ));
        }

        ret = ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &_bits);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not set spidev bits per word to %d for spidev %s")
                    % uint16_t(_bits) % device
            ));
        }

        ret = ioctl(_fd, SPI_IOC_RD_BITS_PER_WORD, &_bits);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not get spidev bits per word for spidev %s")
                    % device
            ));
        }

        ret = ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not set spidev max speed to %d for spidev %s")
                    % _speed % device
            ));
        }

        ret = ioctl(_fd, SPI_IOC_RD_MAX_SPEED_HZ, &_speed);
        if (ret == -1) {
            throw std::runtime_error(str(
                    boost::format("Could not get spidev max speed for spidev %s")
                    % device
            ));
        }
    }

    ~spidev_iface_impl()
    {
        close(_fd);
    }

    uint32_t transact_spi(
            int /* which_slave */,
            const uhd::spi_config_t & /* config */,
            uint32_t data,
            size_t num_bits,
            bool readback
    ) {
        int ret(0);

        uint8_t *tx_data = reinterpret_cast<uint8_t *>(&data);

        assert(num_bits == 24);
        uint8_t tx[] = {tx_data[2], tx_data[1], tx_data[0]};

        uint8_t rx[3]; // Buffer length must match tx buffer

        struct spi_ioc_transfer tr;
        tr.tx_buf = (unsigned long) &tx[0];
        tr.rx_buf = (unsigned long) &rx[0];
        tr.len = num_bits >> 3;
        tr.bits_per_word = _bits;
        tr.tx_nbits = 1; // Standard SPI
        tr.rx_nbits = 1; // Standard SPI
        tr.speed_hz = _speed;
        tr.delay_usecs = _delay;

        ret = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
            throw std::runtime_error("Could not send spidev message");

        return rx[2]; // Assumes that only a single byte is being read.
    }

    uint32_t read_spi(
            int which_slave,
            const uhd::spi_config_t &config,
            uint32_t data,
            size_t num_bits
    ) {
        return transact_spi(
                which_slave, config, data, num_bits, true
        );
    }

    void write_spi(
            int which_slave,
            const uhd::spi_config_t &config,
            uint32_t data,
            size_t num_bits
    ) {
        transact_spi(
                which_slave, config, data, num_bits, false
        );
    }

private:
    int _fd;
    uint32_t _mode = SPI_CPHA | SPI_CPOL;
    uint32_t _speed = 2000000;
    uint8_t _bits = 8;
    uint16_t _delay = 0;
};

/******************************************************************************
 * Factory
 *****************************************************************************/
spidev_iface::sptr spidev_iface::make(
        const std::string &device
) {
    return sptr(new spidev_iface_impl(device));
}

