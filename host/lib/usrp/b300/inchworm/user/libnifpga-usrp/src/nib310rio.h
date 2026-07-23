/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
#ifndef __NIB310RIO_H__
#define __NIB310RIO_H__

#include <linux/types.h>

#define NIB310RIO_IOC_MAGIC (93)

struct ioctl_nib310rio_array
{
    __u32 offset;
    __u32 count;
    __u32 data[0];
};

struct ioctl_nib310rio_fifo_set_buf
{
    __u32 fd;
};

struct ioctl_nib310rio_fifo_acquire
{
    __aligned_u64 elements;
    __aligned_u64 available;
    __u32 timeout_ms;
    __u32 timed_out;
};

struct ioctl_nib310rio_irq_wait
{
    __u32 ctx;
    __u32 mask;
    __u32 timeout_ms;
    __u32 asserted;
    __u32 timed_out;
};

struct ioctl_nib310rio_reg32
{
    __u32 offset;
    __u32 value;
};

struct ioctl_nib310rio_reg64
{
    __u32 offset;
    __u64 value;
};

#define NIB310RIO_IOC_FIFO_STOP  _IO(NIB310RIO_IOC_MAGIC, 2)
#define NIB310RIO_IOC_FIFO_START _IO(NIB310RIO_IOC_MAGIC, 3)
#define NIB310RIO_IOC_FIFO_SET_BUF \
    _IOW(NIB310RIO_IOC_MAGIC, 4, struct ioctl_nib310rio_fifo_set_buf)
#define NIB310RIO_IOC_FIFO_ACQUIRE \
    _IOWR(NIB310RIO_IOC_MAGIC, 5, struct ioctl_nib310rio_fifo_acquire)
#define NIB310RIO_IOC_FIFO_RELEASE   _IOW(NIB310RIO_IOC_MAGIC, 6, __u64)
#define NIB310RIO_IOC_FIFO_GET_AVAIL _IOR(NIB310RIO_IOC_MAGIC, 7, __u64)
#define NIB310RIO_IOC_IRQ_CTX_ALLOC  _IOR(NIB310RIO_IOC_MAGIC, 10, __u32)
#define NIB310RIO_IOC_IRQ_CTX_FREE   _IOW(NIB310RIO_IOC_MAGIC, 11, __u32)
#define NIB310RIO_IOC_IRQ_WAIT \
    _IOWR(NIB310RIO_IOC_MAGIC, 12, struct ioctl_nib310rio_irq_wait)
#define NIB310RIO_IOC_IRQ_ACK _IOW(NIB310RIO_IOC_MAGIC, 13, __u32)
#define NIB310RIO_IOC_PEEK32  _IOWR(NIB310RIO_IOC_MAGIC, 20, struct ioctl_nib310rio_reg32)
#define NIB310RIO_IOC_POKE32  _IOW(NIB310RIO_IOC_MAGIC, 21, struct ioctl_nib310rio_reg32)
#define NIB310RIO_IOC_PEEK64  _IOWR(NIB310RIO_IOC_MAGIC, 22, struct ioctl_nib310rio_reg64)
#define NIB310RIO_IOC_POKE64  _IOW(NIB310RIO_IOC_MAGIC, 23, struct ioctl_nib310rio_reg64)

#endif
