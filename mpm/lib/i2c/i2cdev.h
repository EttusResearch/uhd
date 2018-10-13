/*
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _I2CDEV_H_FOUND_
#define _I2CDEV_H_FOUND_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * This API provides access to i2c devices. It uses the character device API
 * from the Linux kernel and i2c-tools
 *
 * The kernel documentation can be found at
 * https://www.kernel.org/doc/Documentation/i2c/dev-interface
 */

/*! Initialize a i2cdev interface
 *
 * \param fd Return value of the file descriptor
 * \param device Path of i2cdev device, e.g. "/dev/i2c0"
 * \param timeout_ms Timeout to wait for ACK in ms
 *
 * \returns 0 if all is good, or an error code otherwise
 */
int i2cdev_open(int *fd, const char *device, const unsigned int timeout_ms);

/*! Do an i2c transaction over i2cdev
 * If both tx and rx are to be done in one transaction, first tx data is
 * transmitted, followed by a repeated start condition, then the rx data is
 * read.
 *
 * \param fd File descriptor for the i2cdev bus segment
 * \param addr i2c device address
 * \param ten_bit_addr Nonzero if true (typically 0)
 * \param tx Buffer of data to be written to device
 * \param tx_len Total number of non-addr bytes to be written
 * \param rx Buffer where read data can be stored
 * \param rx_len Total number of bytes to be read
 *
 * Assumption: spidev was configured properly beforehand.
 *
 * \returns 0 if all is golden
 */
int i2cdev_transfer(int fd, uint16_t addr, int ten_bit_addr,
                    uint8_t *tx, size_t tx_len,
                    uint8_t *rx, size_t rx_len);
#ifdef __cplusplus
}
#endif
#endif /* _I2CDEV_H_FOUND_ */
