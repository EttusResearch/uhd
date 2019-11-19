//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "i2cdev.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

int i2cdev_open(int *fd, const char *device, const unsigned int timeout_ms)
{
    if (!fd)
    {
        fprintf(stderr, "%s: was passed a null pointer\n",
            __func__);
        return -EINVAL;
    }

    *fd = open(device, O_RDWR | O_LARGEFILE);
    if (*fd < 0) {
        fprintf(stderr, "%s: Failed to open device. %s\n",
            __func__, strerror(*fd));
        return *fd;
    }

    if (ioctl(*fd, I2C_TIMEOUT, timeout_ms) < 0)
    {
        int err = errno;
        fprintf(stderr, "%s: Failed to set timeout. %s\n",
            __func__, strerror(err));
        return err;
    }

    return 0;
}

int i2cdev_transfer(int fd, uint16_t addr, int ten_bit_addr,
                    uint8_t *tx, size_t tx_len,
                    uint8_t *rx, size_t rx_len)
{
    int err;
    struct i2c_msg msgs[2];
    int num_msgs = 0;
    struct i2c_rdwr_ioctl_data i2c_data = {
        .msgs = msgs,
    };

    if (tx && tx_len > 0) {
        msgs[num_msgs].addr = addr;
        msgs[num_msgs].buf = tx;
        msgs[num_msgs].len = tx_len;
        msgs[num_msgs].flags = ten_bit_addr ? I2C_M_TEN : 0;
        num_msgs++;
    }

    if (rx && rx_len > 0) {
        msgs[num_msgs].addr = addr;
        msgs[num_msgs].buf = rx;
        msgs[num_msgs].len = rx_len;
        msgs[num_msgs].flags = ten_bit_addr ? I2C_M_TEN : 0;
        msgs[num_msgs].flags |= I2C_M_RD;
        num_msgs++;
    }

    i2c_data.nmsgs = num_msgs;
    if (num_msgs <= 0)
        return -EINVAL;

    err = ioctl(fd, I2C_RDWR, &i2c_data);
    if (err < 0) {
        fprintf(stderr, "%s: Failed I2C_RDWR: %d\n", __func__, err);
        perror("ioctl: \n");
        return err;
    }

    return 0;
}

