//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

namespace nirio_driver_iface {

nirio_status rio_open(
    const std::string& device_path,
    rio_dev_handle_t& device_handle)
{
    device_handle = ::open(device_path.c_str(), O_RDWR | O_CLOEXEC);
    return (device_handle < 0) ? NiRio_Status_InvalidParameter : NiRio_Status_Success;
}

void rio_close(rio_dev_handle_t& device_handle)
{
    ::close(device_handle);
    device_handle = -1;
}

bool rio_isopen(rio_dev_handle_t device_handle)
{
    return (device_handle >= 0);
}

nirio_status rio_ioctl(
    rio_dev_handle_t device_handle,
    uint32_t ioctl_code,
    const void *write_buf,
    size_t write_buf_len,
    void *read_buf,
    size_t read_buf_len)
{
    nirio_ioctl_block_t ioctl_block = {0,0,0,0,0,0};

    // two-casts necessary to prevent pointer sign-extension
    ioctl_block.in_buf      = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(write_buf));
    ioctl_block.in_buf_len  = write_buf_len;
    ioctl_block.out_buf     = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(read_buf));
    ioctl_block.out_buf_len = read_buf_len;

    int status = ::ioctl(device_handle, ioctl_code, &ioctl_block);
    if (status == -1) {
        switch (errno) {
            case EINVAL:    return NiRio_Status_InvalidParameter;
            case EFAULT:    return NiRio_Status_MemoryFull;
            default:        return NiRio_Status_SoftwareFault;
        }
    } else {
        return NiRio_Status_Success;
    }
}

nirio_status rio_mmap(
    rio_dev_handle_t device_handle,
    uint16_t memory_type,
    size_t size,
    bool writable,
    rio_mmap_t &map)
{
    int access_mode = PROT_READ;    //Write-only mode not supported
    if (writable) access_mode |= PROT_WRITE;
    map.addr = ::mmap(NULL, size, access_mode, MAP_SHARED, device_handle, (off_t) memory_type * sysconf(_SC_PAGESIZE));
    map.size = size;

    if (map.addr == MAP_FAILED)    {
        map.addr = NULL;
        map.size = 0;
        switch (errno) {
            case EINVAL:    return NiRio_Status_InvalidParameter;
            case EFAULT:    return NiRio_Status_MemoryFull;
            default:        return NiRio_Status_SoftwareFault;
        }
    }
    return NiRio_Status_Success;
}

nirio_status rio_munmap(rio_mmap_t &map)
{
    nirio_status status = 0;
    if (map.addr != NULL) {
        status = ::munmap(map.addr, map.size);

        map.addr = NULL;
        map.size = 0;
    }
    return status;
}

}
