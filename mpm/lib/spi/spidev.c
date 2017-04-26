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

#include "spidev.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <stdio.h>

int init_spi(int *fd, const char *device,
    const uint32_t mode,
    const uint32_t speed_hz,
    const uint8_t bits_per_word,
    const uint16_t delay_us
) {
    int err;

    *fd = open(device, O_RDWR);
    if (*fd < 0) {
        fprintf(stderr, "%s: Failed to open device\n", __func__);
        return err;
    }

    uint32_t requested_mode = mode;
    err = ioctl(*fd, SPI_IOC_WR_MODE32, &mode);
    if (err < 0) {
        fprintf(stderr, "%s: Failed to set mode\n", __func__);
        return err;;
    }

    err = ioctl(*fd, SPI_IOC_RD_MODE32, &mode);
    if (err < 0) {
        fprintf(stderr, "%s: Failed to get mode\n", __func__);
        return err;
    }
    if (requested_mode != mode) {
        return 2;
    }

    uint8_t requested_bits_per_word;
    err = ioctl(*fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if (err < 0) {
        fprintf(stderr, "%s: Failed to set bits per word\n", __func__);
        return err;
    }
    err = ioctl(*fd, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word);
    if (err) {
        fprintf(stderr, "%s: Failed to get bits per word\n", __func__);
        return err;
    }
    if (requested_bits_per_word != bits_per_word) {
        return 2;
    }

    uint32_t requested_speed_hz = speed_hz;
    err = ioctl(*fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz);
    if (err < 0) {
        fprintf(stderr, "%s: Failed to set speed\n", __func__);
        return err;
    }
    err = ioctl(*fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz);
    if (err < 0) {
        fprintf(stderr, "%s: Failed to get speed\n", __func__);
        return err;
    }
    if (requested_speed_hz != speed_hz) {
        return 2;
    }

    return 0;
}

int transfer(
        int fd,
        uint8_t *tx, uint8_t *rx, uint32_t len,
        uint32_t speed_hz, uint8_t bits_per_word, uint16_t delay_us
) {
    int err;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long) tx,
        .rx_buf = (unsigned long) rx,
        .len = len,
        .speed_hz = speed_hz,
        .delay_usecs = delay_us,
        .bits_per_word = bits_per_word,
        .cs_change = 0,
        .tx_nbits = 1, // Standard SPI
        .rx_nbits = 1 // Standard SPI
    };

    err = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (err < 0) {
        fprintf(stderr, "%s: Failed ioctl: %d\n", __func__, err);
        perror("ioctl: \n");
	return err;
    }

    return 0;
}

