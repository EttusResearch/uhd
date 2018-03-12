//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include "e300_spi.hpp"

#ifdef E300_NATIVE
#include <boost/thread.hpp>
#include <boost/format.hpp>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

namespace uhd { namespace usrp { namespace e300 {

class spidev_impl : public spi
{
public:

    spidev_impl(const std::string &device)
                          : _mode(SPI_CPHA),
                            _speed(2000000),
                            _bits(8),
                            _delay(0)
    {
        int ret;
        _fd = open(device.c_str(), O_RDWR);
        if (_fd < 0)
            throw uhd::runtime_error(str(boost::format("Could not open spidev device %s") % device));

        ret = ioctl(_fd, SPI_IOC_WR_MODE, &_mode);
        if (ret == -1)
            throw uhd::runtime_error("Could not set spidev mode");

        ret = ioctl(_fd, SPI_IOC_RD_MODE, &_mode);
        if (ret == -1)
            throw uhd::runtime_error("Could not get spidev mode");

        ret = ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &_bits);
        if (ret == -1)
            throw uhd::runtime_error("Could not set spidev bits per word");

        ret = ioctl(_fd, SPI_IOC_RD_BITS_PER_WORD, &_bits);
        if (ret == -1)
            throw uhd::runtime_error("Could not get spidev bits per word");

        ret = ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed);
        if (ret == -1)
            throw uhd::runtime_error("Could not set spidev max speed");

        ret = ioctl(_fd, SPI_IOC_RD_MAX_SPEED_HZ, &_speed);
        if (ret == -1)
            throw uhd::runtime_error("Could not get spidev max speed");
    }

    virtual ~spidev_impl()
    {
        close(_fd);
    }

    uint32_t transact_spi(int, const uhd::spi_config_t &,
                                 uint32_t data, size_t num_bits,
                                 bool)
    {
        int ret(0);
        struct spi_ioc_transfer tr;

        uint8_t *tx_data = reinterpret_cast<uint8_t *>(&data);


        UHD_ASSERT_THROW(num_bits == 24);
        uint8_t tx[] = {tx_data[2], tx_data[1], tx_data[0]};

        uint8_t rx[3];
        tr.tx_buf = (unsigned long) &tx[0];
        tr.rx_buf = (unsigned long) &rx[0];
        tr.len = num_bits >> 3;
        tr.bits_per_word = _bits;
        tr.tx_nbits = 1;
        tr.rx_nbits = 1;
        tr.speed_hz = _speed;
        tr.delay_usecs = _delay;

        ret = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
            throw uhd::runtime_error("Could not send spidev message");

        return rx[2];
    }

private:
    int _fd;
    uint8_t _mode;
    uint32_t _speed;
    uint8_t _bits;
    uint16_t _delay;
};

spi::sptr spi::make(const std::string &device)
{
    return spi::sptr(new spidev_impl(device));
}
}}};
#else
namespace uhd { namespace usrp { namespace e300 {

spi::sptr spi::make(const std::string &)
{
    throw uhd::assertion_error("spi::make() !E300_NATIVE");
}
}}};
#endif //E300_NATIVE
