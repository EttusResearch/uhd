//
// Copyright 2013-2015 Ettus Research LLC
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
#elif !defined(UHD_PLATFORM_LINUX)
    #include <IOKit/IOKitLib.h>
#endif

// CTL_CODE macro for non-win OSes
#ifndef UHD_PLATFORM_WIN32
    #define CTL_CODE(a,controlCode,b,c) (controlCode)
#endif

namespace nirio_driver_iface {

const uint32_t NIRIO_IOCTL_BASE = 0x800;

const uint32_t NIRIO_IOCTL_SYNCOP =
   CTL_CODE(FILE_DEVICE_UNKNOWN,
            NIRIO_IOCTL_BASE + 4,
            METHOD_OUT_DIRECT,
            FILE_READ_DATA | FILE_WRITE_DATA);
                                ///< The synchronous operation code. Note: We
                                /// must use METHOD_OUT_DIRECT on the syncOp()
                                /// IOCTL to ensure the contents of the output
                                /// block are available in the kernel.

const uint32_t NIRIO_IOCTL_GET_IFACE_NUM =
   CTL_CODE(FILE_DEVICE_UNKNOWN,
            NIRIO_IOCTL_BASE + 6,
            METHOD_BUFFERED,
            FILE_READ_DATA);    ///< Get the interface number for a device

const uint32_t NIRIO_IOCTL_GET_SESSION =
   CTL_CODE(FILE_DEVICE_UNKNOWN,
            NIRIO_IOCTL_BASE + 8,
            METHOD_BUFFERED,
            FILE_READ_ACCESS);  ///< Gets a previously opened session to a device

const uint32_t NIRIO_IOCTL_POST_OPEN =
   CTL_CODE(FILE_DEVICE_UNKNOWN,
            NIRIO_IOCTL_BASE + 9,
            METHOD_BUFFERED,
            FILE_READ_ACCESS);  ///< Called after opening a session

const uint32_t NIRIO_IOCTL_PRE_CLOSE =
   CTL_CODE(FILE_DEVICE_UNKNOWN,
            NIRIO_IOCTL_BASE + 10,
            METHOD_BUFFERED,
            FILE_READ_ACCESS);  ///< Called before closing a session


//Device handle definition
#if defined(UHD_PLATFORM_LINUX)
    typedef int rio_dev_handle_t;
#elif defined(UHD_PLATFORM_WIN32)
    typedef HANDLE rio_dev_handle_t;
#else
    typedef io_connect_t rio_dev_handle_t;
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
