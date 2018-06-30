//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "spidev.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
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
    if (!fd)
    {
        fprintf(stderr, "%s: was passed a null pointer\n",
            __func__);
        return -EINVAL;
    }

    *fd = open(device, O_RDWR);
    if (*fd < 0) {
        fprintf(stderr, "%s: Failed to open device. %s\n",
            __func__, strerror(*fd));
        return *fd;
    }

    // SPI_IOC_WR commands interpret our pointer as non-const, so play it safe
    uint32_t set_mode = mode;
    if (ioctl(*fd, SPI_IOC_WR_MODE32, &set_mode) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to set mode. %s\n",
            __func__, strerror(err));
        return err;
    }

    uint32_t coerced_mode;
    if (ioctl(*fd, SPI_IOC_RD_MODE32, &coerced_mode) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to get mode. %s\n",
            __func__, strerror(err));
        return err;
    }

    if (coerced_mode != mode) {
        fprintf(stderr, "%s: Failed to set mode to %d, got %d\n",
            __func__, mode, coerced_mode);
        return -ENOTSUP;
    }

    uint8_t set_bits_per_word = bits_per_word;
    if (ioctl(*fd, SPI_IOC_WR_BITS_PER_WORD, &set_bits_per_word) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to set bits per word. %s\n",
            __func__, strerror(err));
        return err;
    }

    uint8_t coerced_bits_per_word;
    if (ioctl(*fd, SPI_IOC_RD_BITS_PER_WORD, &coerced_bits_per_word) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to get bits per word (%s)\n",
            __func__, strerror(err));
        return err;
    }

    if (coerced_bits_per_word != bits_per_word) {
        fprintf(stderr, "%s: Failed to set bits per word to %d, got %d\n",
            __func__, bits_per_word, coerced_bits_per_word);
        return -ENOTSUP;
    }

    uint32_t set_speed_hz = speed_hz;
    if (ioctl(*fd, SPI_IOC_WR_MAX_SPEED_HZ, &set_speed_hz) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to set speed (%s)\n",
            __func__, strerror(err));
        return err;
    }

    uint32_t coerced_speed_hz;
    if (ioctl(*fd, SPI_IOC_RD_MAX_SPEED_HZ, &coerced_speed_hz) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to get speed (%s)\n",
            __func__, strerror(err));
        return err;
    }

    if (coerced_speed_hz != speed_hz) {
        fprintf(stderr, "%s: Failed to set speed to %d, got %d\n",
            __func__, speed_hz, coerced_speed_hz);
        return -ENOTSUP;
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

