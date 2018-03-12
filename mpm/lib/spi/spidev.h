/*
 * Copyright 2017 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>

/*! Initialize a spidev interface
 *
 * \param fd Return value of the file descriptor
 * \param device Path of spidev device, e.g. "/dev/spidev0.0"
 * \param mode SPI mode. See linux/spi/spidev.h
 * \param speed_hz The *maximum* SPI speed in Hz
 * \param bits_per_word Just set it to 8.
 * \param delay_us Delay between writes in microseconds
 *
 * \returns 0 if all is good, or an error code otherwise
 */
int init_spi(int *fd, const char *device,
    const uint32_t mode,
    const uint32_t speed_hz,
    const uint8_t bits_per_word,
    const uint16_t delay_us
);

/*! Do a SPI transaction over spidev
 *
 * \param tx Buffer of data to be written
 * \param rx Must match tx buffer length; result will be written here
 * \param len Total number of bytes in this transaction
 * \param speed_hz Speed of this transaction in Hz
 * \param bits_per_word 8, dude
 * \param delay_us Delay between transfers
 *
 * Assumption: spidev was configured properly beforehand.
 *
 * \returns 0 if all is golden
 */
int transfer(
        int fd,
        uint8_t *tx, uint8_t *rx, uint32_t len,
        uint32_t speed_hz, uint8_t bits_per_word, uint16_t delay_us
);

