//
// Copyright 2013,2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <process.h>

#define NIRIO_IOCTL_MAP_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0xF00, METHOD_BUFFERED, (FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define NIRIO_IOCTL_UNMAP_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0xF01, METHOD_BUFFERED, (FILE_READ_ACCESS | FILE_WRITE_ACCESS))

namespace nirio_driver_iface {

nirio_status rio_open(
    const std::string& device_path,
    rio_dev_handle_t& device_handle)
{
    device_handle = CreateFileA(device_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,  /* default security */
                                OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL /* template file */);

    return (device_handle == INVALID_HANDLE_VALUE) ? NiRio_Status_InvalidParameter : NiRio_Status_Success;
}

void rio_close(rio_dev_handle_t& device_handle)
{
    ::CloseHandle(device_handle);
    device_handle = INVALID_HANDLE_VALUE;
}

bool rio_isopen(rio_dev_handle_t device_handle)
{
    return (device_handle != INVALID_HANDLE_VALUE);
}

nirio_status rio_ioctl(
    rio_dev_handle_t device_handle,
    uint32_t ioctl_code,
    const void *write_buf,
    size_t write_buf_len,
    void *read_buf,
    size_t read_buf_len)
{
    if (!rio_isopen(device_handle)) return NiRio_Status_ResourceNotInitialized;

    /* Note, if the file handle was opened with the OVERLAPPED flag,  you must
    * supply an OVERLAPPED structure to ReadFile, WriteFile, and
    * DeviceIoControl, even when doing synchronous IO. */
    OVERLAPPED zeroedOverlapped = {0};
    DWORD outLen = 0;

    if (!(DeviceIoControl(device_handle, ioctl_code,
            const_cast<void*>(write_buf), static_cast<DWORD>(write_buf_len),
            read_buf, static_cast<DWORD>(read_buf_len),
            &outLen, &zeroedOverlapped )))
    {
        UHD_UNUSED(int_fast32_t lastError) = GetLastError();
        return NiRio_Status_SoftwareFault;
    }

    return NiRio_Status_Success;
}

unsigned int __stdcall memory_map_thread_routine(void *context)
{
    rio_mmap_threadargs_t *args = (rio_mmap_threadargs_t*)context;
    args->status = rio_ioctl(args->device_handle, NIRIO_IOCTL_MAP_MEMORY, &(args->params), sizeof(args->params), NULL, 0);
    if (nirio_status_fatal(args->status))
    {
        SetEvent(reinterpret_cast<HANDLE>(args->params.map_ready_event_handle));
    }
    return 0;
}

nirio_status rio_mmap(
    rio_dev_handle_t device_handle,
    uint16_t memory_type,
    size_t size,
    bool writable,
    rio_mmap_t &map)
{
    if (!rio_isopen(device_handle)) return NiRio_Status_ResourceNotInitialized;

    access_mode_t access_mode = writable ? ACCESS_MODE_WRITE : ACCESS_MODE_READ;

    uint64_t mapped_addr = 0;
    map.map_thread_args.device_handle = device_handle;
    map.map_thread_args.status = NiRio_Status_Success;
    map.map_thread_args.params.memoryType = memory_type;
    map.map_thread_args.params.size = (uint32_t)size;
    map.map_thread_args.params.mapped_va_ptr = reinterpret_cast<uintptr_t>(&mapped_addr);
    map.map_thread_args.params.access_mode = (uint8_t)access_mode;
    HANDLE map_ready_event_handle = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (map_ready_event_handle == NULL) {
        map.addr = NULL;
        return NiRio_Status_SoftwareFault;
    }
    map.map_thread_args.params.map_ready_event_handle = reinterpret_cast<uint64_t>(map_ready_event_handle);
    map.map_thread_handle = (HANDLE) _beginthreadex(NULL, 0, memory_map_thread_routine, &(map.map_thread_args), 0, NULL);

    nirio_status status = NiRio_Status_Success;
    if (map.map_thread_handle == NULL) {
        map.addr = NULL;
        return NiRio_Status_SoftwareFault;
    } else {
        WaitForSingleObject(map_ready_event_handle, INFINITE);
        map.addr = reinterpret_cast<void*>(mapped_addr);
        if (map.addr == NULL) {
            WaitForSingleObject(map.map_thread_handle, INFINITE);
            CloseHandle(map.map_thread_handle);
            nirio_status_chain(map.map_thread_args.status, status);
        }
    }
    CloseHandle(map_ready_event_handle);
    return status;
}

nirio_status rio_munmap(rio_mmap_t &map)
{
    if (!rio_isopen(map.map_thread_args.device_handle)) return NiRio_Status_ResourceNotInitialized;

    nirio_status status = NiRio_Status_Success;
    if (map.addr != NULL) {
        uint64_t mapped_addr = reinterpret_cast<uintptr_t>(map.addr);
        status = rio_ioctl(map.map_thread_args.device_handle, NIRIO_IOCTL_UNMAP_MEMORY, &mapped_addr, sizeof(mapped_addr), NULL, 0);
        if (nirio_status_not_fatal(status)) {
            WaitForSingleObject(map.map_thread_handle, INFINITE);
        }
        CloseHandle(map.map_thread_handle);
        map.addr = NULL;
    }
    return status;
}

}
