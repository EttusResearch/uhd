//
// Copyright 2013 Ettus Research LLC
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
    #include <Windows.h>
    #pragma warning(disable:4201)  // nonstandard extension used : nameless struct/union
        #include <WinIoCtl.h>
    #pragma warning(default:4201)
#elif defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
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


// -------------------------------
// Function Codes: defined as integers rather than enums because they
// are going to be carried accross boundaries so size matters

namespace NIRIO_FUNC
{
   const uint32_t GET32             = 0x00000001;
   const uint32_t SET32             = 0x00000002;
   const uint32_t SET_DRIVER_CONFIG = 0x00000007;
   const uint32_t FIFO              = 0x00000008;
   const uint32_t IO                = 0x0000000A;
   const uint32_t FIFO_STOP_ALL     = 0x0000000C;
   const uint32_t ADD_RESOURCE      = 0x0000000D;
   const uint32_t GET_STRING        = 0x0000000E;
   const uint32_t SET_STRING        = 0x0000000F;
   const uint32_t DOWNLOAD          = 0x00000013;
   const uint32_t RESET             = 0x00000014;
}

namespace NIRIO_RESOURCE
{
   const uint32_t INPUT_FIFO    = 0xD0000001;
   const uint32_t OUTPUT_FIFO   = 0xD0000002;
}

namespace NIRIO_FIFO
{
   const uint32_t CONFIGURE = 0x80000001;
   const uint32_t START     = 0x80000002;
   const uint32_t STOP      = 0x80000003;
   const uint32_t READ      = 0x80000004;
   const uint32_t WRITE     = 0x80000005;
   const uint32_t WAIT      = 0x80000006;
   const uint32_t GRANT     = 0x80000007;
}

namespace NIRIO_IO
{
   const uint32_t POKE64             = 0xA0000005;
   const uint32_t POKE32             = 0xA0000006;
   const uint32_t POKE16             = 0xA0000007;
   const uint32_t POKE8              = 0xA0000008;
   const uint32_t PEEK64             = 0xA0000009;
   const uint32_t PEEK32             = 0xA000000A;
   const uint32_t PEEK16             = 0xA000000B;
   const uint32_t PEEK8              = 0xA000000C;
   const uint32_t READ_BLOCK         = 0xA000000D;
   const uint32_t WRITE_BLOCK        = 0xA000000E;
   const uint32_t GET_IO_WINDOW      = 0xA000000F;
   const uint32_t GET_IO_WINDOW_SIZE = 0xA0000010;
}

struct nirio_ioctl_packet_t {
    nirio_ioctl_packet_t(
        void* const _outBuf,
        const uint32_t _outSize,
        const int32_t _statusCode)
    {
        outBuf._64BitField = 0;
        outBuf.pointer = _outBuf;
        outSize    = _outSize;
        statusCode = _statusCode;
    };

    union {
        void* pointer;
        uint64_t _64BitField;
    } outBuf;

    uint32_t outSize;
    int32_t statusCode;
};

struct nirio_ioctl_block_t
{
    uint64_t inBuf;
    uint64_t outBuf;
    uint32_t inBufLength;
    uint32_t outBufLength;
    uint32_t bytesReturned;
    uint32_t padding;
};

struct nirio_syncop_in_params_t
{
   uint32_t function;
   uint32_t subfunction;

   union
   {
      struct
      {
         uint32_t attribute;
         uint32_t value;
      } attribute32;

      struct
      {
         uint32_t attribute;
         uint64_t value;
      } attribute64;

      struct
      {
         uint32_t attribute;
      } attributeStr;

      struct
      {
         uint32_t attribute;
      } download;

      union
      {
         struct
         {
            uint32_t reserved_field_0_0_0;
         } reserved_field_0_0;
         struct
         {
            uint32_t reserved_field_0_1_0;
            uint32_t reserved_field_0_1_1;
         } reserved_field_0_1;
         struct
         {
            uint32_t reserved_field_0_2_0;
         } reserved_field_0_2;
      } reserved_field_0;

      union
      {
         struct
         {
            uint32_t channel;
            uint32_t baseAddress;
            uint32_t depthInSamples;
            uint32_t version;
         } fifo;
         struct
         {
            uint32_t channel;
            uint32_t baseAddress;
            uint32_t depthInSamples;
            uint32_t version;
            uint32_t scalarType;
            uint32_t bitWidth;
         } fifoWithDataType;
         struct
         {
            uint64_t rangeBaseAddress;
            uint32_t rangeSizeInBytes;
            uint32_t rangeAttribute;
         } atomic; // obsolete
      } add;

      struct
      {
         uint32_t channel;

         union
         {
            struct
            {
               uint32_t requestedDepth;
               uint8_t  requiresActuals;
            } config;
            struct
            {
               uint32_t timeout;
            } read;
            struct
            {
               uint32_t timeout;
               uint32_t scalarType;
               uint32_t bitWidth;
            } readWithDataType;
            struct
            {
               uint32_t timeout;
            } write;
            struct
            {
               uint32_t timeout;
               uint32_t scalarType;
               uint32_t bitWidth;
            } writeWithDataType;
            struct
            {
               uint32_t elementsRequested;
               uint32_t scalarType;
               uint32_t bitWidth;
               uint32_t timeout;
               uint8_t  output;
            } wait;
            struct
            {
               uint32_t elements;
            } grant;
         } op;
      } fifo;

      struct
      {
         uint64_t reserved_field_1_0;
         uint32_t reserved_field_1_1;
         uint32_t reserved_field_1_2;
      } reserved_field_1; // Obsolete

      struct
      {
         uint32_t offset;
         union
         {
            uint64_t value64;
            uint32_t value32;
            uint16_t value16;
            uint8_t  value8;
         } value;
         union
         {
            uint32_t   sizeToMap;
         } memoryMappedIoWindow;
      } io;

      struct
      {
         uint32_t reserved_field_2_0;
         uint32_t reserved_field_2_1;
      } reserved_field_2;

      struct
      {
         uint32_t reserved_field_3_0;
      } reserved_field_3;

      union
      {
         struct
         {
            uint32_t reserved_field_4_0;
            int32_t  reserved_field_4_1;
         } wait;
      } reserved_field_4;

   } params;

   uint32_t inbufByteLen;

   union
   {
      const void* pointer;
      uint64_t    _64BitField;
   } inbuf;
};

static inline void init_syncop_in_params(nirio_syncop_in_params_t& param, const void* const buf, const uint32_t len)
{
   param.inbuf._64BitField = 0;
   param.inbuf.pointer = buf;
   param.inbufByteLen = len;
}


struct nirio_syncop_out_params_t
{
   union
   {
      struct
      {
         uint32_t value;
      } attribute32;

      struct
      {
         uint64_t value;
      } attribute64;

      union
      {
         struct
         {
            uint32_t reserved_field_0_0;
         } enable;
      } reserved_field_0;

      struct
      {
         union
         {
            struct
            {
               uint32_t actualDepth;
               uint32_t actualSize;
            } config;
            struct
            {
               uint32_t numberRead;
               uint32_t numberRemaining;
            } read;
            struct
            {
               uint32_t numberRemaining;
            } write;
            struct
            {
               union
               {
                  void*    pointer;
                  uint64_t _64BitField;
               } elements;
            } wait;
         } op;
      } fifo;

      struct
      {
        union
         {
            union
            {
               uint64_t value64;
               uint32_t value32;
               uint16_t value16;
               uint8_t  value8;
            } value;
            union
            {
               void*    memoryMappedAddress;
               uint64_t _64BitField;
            } memoryMappedIoWindow;
            union
            {
               uint32_t   size;
            } memoryMappedIoWindowSize;
         };
      } io;

      uint32_t stringLength;

      struct
      {
         uint32_t reserved_field_1_0;
      } reserved_field_1;

   } params;

   uint32_t    outbufByteLen;

   union
   {
      void*    pointer;
      uint64_t _64BitField;
   } outbuf;
};

static inline void init_syncop_out_params(nirio_syncop_out_params_t& param, void* buf, uint32_t len)
{
   param.outbuf._64BitField = 0;
   param.outbuf.pointer = buf;
   param.outbufByteLen = len;
}



//Device handle definition
#if defined(UHD_PLATFORM_LINUX)
    typedef int rio_dev_handle_t;
#elif defined(UHD_PLATFORM_WIN32)
    typedef HANDLE rio_dev_handle_t;
#elif defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    typedef io_connect_t rio_dev_handle_t;
#else
    #error OS not supported by nirio_driver_iface.
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
#elif defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
     struct rio_mmap_t {
         rio_mmap_t() : addr(NULL) {}
         void *addr;

         bool is_null() { return addr == NULL; }
     };
#else
    #error OS not supported by nirio_driver_iface.
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
