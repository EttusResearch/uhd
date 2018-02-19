//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_DRIVER_IFACE_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_DRIVER_IFACE_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <uhd/transport/nirio/status.h>
#include <uhd/config.hpp>
#if defined(UHD_PLATFORM_WIN32)
    #include <windows.h>
    #ifdef _MSC_VER
        #pragma warning(disable:4201)  // nonstandard extension used : nameless struct/union
    #endif
        #include <winioctl.h>
    #ifdef _MSC_VER
        #pragma warning(default:4201)
    #endif
#elif defined(UHD_PLATFORM_MACOS)
    #include <IOKit/IOKitLib.h>
#elif defined(UHD_PLATFORM_LINUX)
   #include <linux/ioctl.h>
#endif

#if __GNUC__
    typedef uint64_t  aligned_uint64_t __attribute__ ((aligned(8)));
#else
    typedef uint64_t  aligned_uint64_t;
#endif

//IOCTL Definitions

#if defined(UHD_PLATFORM_WIN32)

    #define IOCTL_ACCESS_ANY       (FILE_ANY_ACCESS)
    #define IOCTL_ACCESS_READ      (FILE_READ_ACCESS)
    #define IOCTL_ACCESS_WRITE     (FILE_WRITE_ACCESS)
    #define IOCTL_ACCESS_RW        (FILE_READ_ACCESS | FILE_WRITE_ACCESS)

    #define IOCTL(type, function, access) \
        CTL_CODE((0x8000+type), (0x800+function), METHOD_BUFFERED, access)

#elif defined(UHD_PLATFORM_MACOS)

    #define IOCTL_ACCESS_ANY       (0U)
    #define IOCTL_ACCESS_READ      (1U)
    #define IOCTL_ACCESS_WRITE     (2U)
    #define IOCTL_ACCESS_RW        (3U)

    #define IOCTL(type, function, access) \
        (((access   & 0x0003) << 30) | \
         ((type     & 0x00FF) << 16) | \
         ((function & 0xFFFF) << 0))

#elif defined(UHD_PLATFORM_LINUX)

    #define IOCTL_ACCESS_ANY       (_IOC_NONE)
    #define IOCTL_ACCESS_READ      (_IOC_READ)
    #define IOCTL_ACCESS_WRITE     (_IOC_WRITE)
    #define IOCTL_ACCESS_RW        (_IOC_READ | _IOC_WRITE)

    struct nirio_ioctl_block_t {
        aligned_uint64_t in_buf;
        aligned_uint64_t out_buf;
        uint32_t in_buf_len;
        uint32_t out_buf_len;
        uint32_t bytes_returned;
        uint32_t padding;
    };

    #define IOCTL(type, function, access) \
        _IOC(access, type, function, sizeof(nirio_ioctl_block_t))

#else

    #define IOCTL_ACCESS_ANY       (0U)
    #define IOCTL_ACCESS_READ      (1U)
    #define IOCTL_ACCESS_WRITE     (2U)
    #define IOCTL_ACCESS_RW        (3U)

    #define IOCTL(type, function, access) \
        (((access   & 0x0003) << 30) | \
         ((type     & 0x00FF) << 16) | \
         ((function & 0xFFFF) << 0))

#endif

namespace nirio_driver_iface {

//Device handle definition
#if defined(UHD_PLATFORM_LINUX)
    typedef int rio_dev_handle_t;
#elif defined(UHD_PLATFORM_WIN32)
    typedef HANDLE rio_dev_handle_t;
#elif defined(UHD_PLATFORM_MACOS)
    typedef io_connect_t rio_dev_handle_t;
#else
    typedef int rio_dev_handle_t;
#endif
static const rio_dev_handle_t INVALID_RIO_HANDLE = ((rio_dev_handle_t)-1);

//Memory mapping container definition
#if defined(UHD_PLATFORM_LINUX)
    struct rio_mmap_t {
        rio_mmap_t() : addr(NULL), size(0) {}
        void *addr;
        size_t size;

        bool is_null() { return (size == 0 || addr == NULL); }
    };
#elif defined(UHD_PLATFORM_WIN32)
    enum access_mode_t {
       ACCESS_MODE_READ,
       ACCESS_MODE_WRITE
    };

    struct rio_mmap_params_t
    {
       uint64_t mapped_va_ptr;
       uint64_t map_ready_event_handle;
       uint32_t size;
       uint16_t memoryType;
       uint8_t access_mode;
    };

    struct rio_mmap_threadargs_t
    {
        rio_dev_handle_t device_handle;
        rio_mmap_params_t params;
        nirio_status status;
    };

    struct rio_mmap_t
    {
        rio_mmap_t() : addr(NULL) {}
        void *addr;
        HANDLE map_thread_handle;
        rio_mmap_threadargs_t map_thread_args;

        bool is_null() { return addr == NULL; }
    };
#else
     struct rio_mmap_t {
         rio_mmap_t() : addr(NULL) {}
         void *addr;

         bool is_null() { return addr == NULL; }
     };
#endif

    nirio_status rio_open(
        const std::string& device_path,
        rio_dev_handle_t& device_handle);

    void rio_close(
        rio_dev_handle_t& device_handle);

    bool rio_isopen(
        rio_dev_handle_t device_handle);

    nirio_status rio_ioctl(
        rio_dev_handle_t device_handle,
        uint32_t ioctl_code,
        const void *write_buf,
        size_t write_buf_len,
        void *read_buf,
        size_t read_buf_len);

    nirio_status rio_mmap(
        rio_dev_handle_t device_handle,
        uint16_t memory_type,
        size_t size,
        bool writable,
        rio_mmap_t &map);

    nirio_status rio_munmap(
        rio_mmap_t &map);
}

#endif
