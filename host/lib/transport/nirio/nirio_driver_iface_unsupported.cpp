//
// Copyright 2013,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/transport/nirio/nirio_driver_iface.h>

namespace nirio_driver_iface {

nirio_status rio_open(
    UHD_UNUSED(const std::string& device_path),
    UHD_UNUSED(rio_dev_handle_t& device_handle))
{
    return NiRio_Status_FeatureNotSupported;
}

void rio_close(UHD_UNUSED(rio_dev_handle_t& device_handle))
{
}

bool rio_isopen(UHD_UNUSED(rio_dev_handle_t device_handle))
{
    return false;
}

nirio_status rio_ioctl(
    UHD_UNUSED(rio_dev_handle_t device_handle),
    UHD_UNUSED(uint32_t ioctl_code),
    UHD_UNUSED(const void *write_buf),
    UHD_UNUSED(size_t write_buf_len),
    UHD_UNUSED(void *read_buf),
    UHD_UNUSED(size_t read_buf_len))
{
    return NiRio_Status_FeatureNotSupported;
}

nirio_status rio_mmap(
    UHD_UNUSED(rio_dev_handle_t device_handle),
    UHD_UNUSED(uint16_t memory_type),
    UHD_UNUSED(size_t size),
    UHD_UNUSED(bool writable),
    UHD_UNUSED(rio_mmap_t &map))
{
    return NiRio_Status_FeatureNotSupported;
}

nirio_status rio_munmap(UHD_UNUSED(rio_mmap_t &map))
{
    return NiRio_Status_FeatureNotSupported;
}

}
