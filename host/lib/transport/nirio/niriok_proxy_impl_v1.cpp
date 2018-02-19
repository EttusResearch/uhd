//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/nirio/niriok_proxy_impl_v1.h>
#include <cstring>

// "push" and "pop" introduced in GCC 4.6; works with all clang
#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

// CTL_CODE macro for non-win OSes
#ifndef UHD_PLATFORM_WIN32
    #define CTL_CODE(a,controlCode,b,c) (controlCode)
#endif

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

//const uint32_t NIRIO_IOCTL_GET_SESSION =
   //CTL_CODE(FILE_DEVICE_UNKNOWN,
            //NIRIO_IOCTL_BASE + 8,
            //METHOD_BUFFERED,
            //FILE_READ_ACCESS);  ///< Gets a previously opened session to a device

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

namespace uhd { namespace niusrprio
{
    // -------------------------------
    // Function Codes: defined as integers rather than enums because they
    // are going to be carried accross boundaries so size matters

    struct NIRIO_FUNC
    {
       static const uint32_t GET32             = 0x00000001;
       static const uint32_t SET32             = 0x00000002;
       static const uint32_t SET_DRIVER_CONFIG = 0x00000007;
       static const uint32_t FIFO              = 0x00000008;
       static const uint32_t IO                = 0x0000000A;
       static const uint32_t FIFO_STOP_ALL     = 0x0000000C;
       static const uint32_t ADD_RESOURCE      = 0x0000000D;
       static const uint32_t GET_STRING        = 0x0000000E;
       static const uint32_t SET_STRING        = 0x0000000F;
       static const uint32_t DOWNLOAD          = 0x00000013;
       static const uint32_t RESET             = 0x00000014;
    };

    struct NIRIO_RESOURCE
    {
       static const uint32_t INPUT_FIFO    = 0xD0000001;
       static const uint32_t OUTPUT_FIFO   = 0xD0000002;
    };

    struct NIRIO_FIFO
    {
       static const uint32_t CONFIGURE = 0x80000001;
       static const uint32_t START     = 0x80000002;
       static const uint32_t STOP      = 0x80000003;
       static const uint32_t READ      = 0x80000004;
       static const uint32_t WRITE     = 0x80000005;
       static const uint32_t WAIT      = 0x80000006;
       static const uint32_t GRANT     = 0x80000007;
    };

    struct NIRIO_IO
    {
       static const uint32_t POKE64             = 0xA0000005;
       static const uint32_t POKE32             = 0xA0000006;
       static const uint32_t POKE16             = 0xA0000007;
       static const uint32_t POKE8              = 0xA0000008;
       static const uint32_t PEEK64             = 0xA0000009;
       static const uint32_t PEEK32             = 0xA000000A;
       static const uint32_t PEEK16             = 0xA000000B;
       static const uint32_t PEEK8              = 0xA000000C;
       static const uint32_t READ_BLOCK         = 0xA000000D;
       static const uint32_t WRITE_BLOCK        = 0xA000000E;
       static const uint32_t GET_IO_WINDOW      = 0xA000000F;
       static const uint32_t GET_IO_WINDOW_SIZE = 0xA0000010;
    };

    struct nirio_ioctl_packet_t {
        nirio_ioctl_packet_t(void* const _outBuf, const uint32_t _outSize, const int32_t _statusCode)
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

    //-------------------------------------------------------
    // niriok_proxy_impl_v1
    //-------------------------------------------------------
    niriok_proxy_impl_v1::niriok_proxy_impl_v1()
    {
    }

    niriok_proxy_impl_v1::~niriok_proxy_impl_v1()
    {
        close();
    }

    nirio_status niriok_proxy_impl_v1::open(const std::string& interface_path)
    {
        WRITER_LOCK

        if (interface_path.empty()) return NiRio_Status_ResourceNotFound;

        //close if already open.
        // use non-locking _close since we already have the lock
        _close();

        nirio_status status = NiRio_Status_Success;
        nirio_status_chain(nirio_driver_iface::rio_open(
            interface_path, _device_handle), status);
        if (nirio_status_not_fatal(status)) {
            nirio_status_chain(nirio_driver_iface::rio_ioctl(_device_handle,
                                        NIRIO_IOCTL_POST_OPEN,
                                        NULL, 0, NULL, 0), status);
            nirio_ioctl_packet_t out(&_interface_num, sizeof(_interface_num), 0);
            nirio_status_chain(nirio_driver_iface::rio_ioctl(_device_handle,
                                        NIRIO_IOCTL_GET_IFACE_NUM,
                                        NULL, 0,
                                        &out, sizeof(out)), status);

            if (nirio_status_fatal(status)) _close();
        }
        return status;
    }

    void niriok_proxy_impl_v1::close(void)
    {
        WRITER_LOCK

        _close();
    }

    // this protected _close doesn't acquire the lock, so it can be used in methods 
    // that already have the lock
    void niriok_proxy_impl_v1::_close()
    {

       if(nirio_driver_iface::rio_isopen(_device_handle))
       {
            nirio_driver_iface::rio_ioctl(
                _device_handle, NIRIO_IOCTL_PRE_CLOSE, NULL, 0, NULL, 0);
            nirio_driver_iface::rio_close(_device_handle);
       }
    }

    nirio_status niriok_proxy_impl_v1::reset()
    {
        READER_LOCK

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::RESET;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::get_version(
        nirio_version_t type,
        uint32_t& major,
        uint32_t& upgrade,
        uint32_t& maintenance,
        char& phase,
        uint32_t& build)
    {   
        nirio_device_attribute32_t version_attr = (type==CURRENT)?RIO_CURRENT_VERSION:RIO_OLDEST_COMPATIBLE_VERSION;
        uint32_t raw_version = 0;
        nirio_status status = get_attribute(version_attr, raw_version);

        major       = (raw_version & VERSION_MAJOR_MASK) >> VERSION_MAJOR_SHIFT;
        upgrade     = (raw_version & VERSION_UPGRD_MASK) >> VERSION_UPGRD_SHIFT;
        maintenance = (raw_version & VERSION_MAINT_MASK) >> VERSION_MAINT_SHIFT;
        build       = (raw_version & VERSION_BUILD_MASK) >> VERSION_BUILD_SHIFT;

        uint32_t phase_num = (raw_version & VERSION_PHASE_MASK) >> VERSION_PHASE_SHIFT;
        switch (phase_num) {
            case 0: phase = 'd'; break;
            case 1: phase = 'a'; break;
            case 2: phase = 'b'; break;
            case 3: phase = 'f'; break;
        }

        return status;
    }

    nirio_status niriok_proxy_impl_v1::sync_operation(
        const void *writeBuffer,
        size_t writeBufferLength,
        void *readBuffer,
        size_t readBufferLength)
    {
        READER_LOCK

        nirio_ioctl_packet_t out(readBuffer, readBufferLength, 0);
        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    NIRIO_IOCTL_SYNCOP,
                                    writeBuffer, writeBufferLength,
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.statusCode;
    }

    nirio_status niriok_proxy_impl_v1::get_attribute(
        const nirio_device_attribute32_t attribute,
        uint32_t& attrValue)
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::GET32;
        in.params.attribute32.attribute = static_cast <uint32_t> (attribute);

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));

        attrValue = out.params.attribute32.value;
        return status;
    }

    nirio_status niriok_proxy_impl_v1::set_attribute(
        const nirio_device_attribute32_t attribute,
        const uint32_t value)
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

       in.function = NIRIO_FUNC::SET32;
       in.params.attribute32.attribute = static_cast <uint32_t> (attribute);
       in.params.attribute32.value  = value;

       return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::peek(uint32_t offset, uint32_t& value)
    {
        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function         = NIRIO_FUNC::IO;
        in.subfunction      = NIRIO_IO::PEEK32;
        in.params.io.offset = offset;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        value = out.params.io.value.value32;
        return status;
    }

    nirio_status niriok_proxy_impl_v1::peek(uint32_t offset, uint64_t& value)
    {
        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function         = NIRIO_FUNC::IO;
        in.subfunction      = NIRIO_IO::PEEK64;
        in.params.io.offset = offset;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        value = out.params.io.value.value64;
        return status;
    }

    nirio_status niriok_proxy_impl_v1::poke(uint32_t offset, const uint32_t& value)
    {
        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function                 = NIRIO_FUNC::IO;
        in.subfunction              = NIRIO_IO::POKE32;
        in.params.io.offset         = offset;
        in.params.io.value.value32  = value;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::poke(uint32_t offset, const uint64_t& value)
    {
        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function                 = NIRIO_FUNC::IO;
        in.subfunction              = NIRIO_IO::POKE64;
        in.params.io.offset         = offset;
        in.params.io.value.value64  = value;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::map_fifo_memory(
        uint32_t fifo_instance,
        size_t size,
        nirio_driver_iface::rio_mmap_t& map)
    {
        READER_LOCK

        return nirio_driver_iface::rio_mmap(_device_handle,
                GET_FIFO_MEMORY_TYPE(fifo_instance),
                size, true, map);
    }

    nirio_status niriok_proxy_impl_v1::unmap_fifo_memory(
        nirio_driver_iface::rio_mmap_t& map)
    {
        READER_LOCK

        return nirio_driver_iface::rio_munmap(map);
    }

    nirio_status niriok_proxy_impl_v1::stop_all_fifos()
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::FIFO_STOP_ALL;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::add_fifo_resource(const nirio_fifo_info_t& fifo_info)
    {
       nirio_syncop_in_params_t in = {};
       nirio_syncop_out_params_t out = {};

       in.function    = NIRIO_FUNC::ADD_RESOURCE;
       if (fifo_info.direction == OUTPUT_FIFO)
           in.subfunction = NIRIO_RESOURCE::OUTPUT_FIFO;
       else
           in.subfunction = NIRIO_RESOURCE::INPUT_FIFO;

       in.params.add.fifoWithDataType.channel        = fifo_info.channel;
       in.params.add.fifoWithDataType.baseAddress    = fifo_info.base_addr;
       in.params.add.fifoWithDataType.depthInSamples = fifo_info.depth;
       in.params.add.fifoWithDataType.scalarType     = static_cast <uint32_t> (fifo_info.scalar_type);
       in.params.add.fifoWithDataType.bitWidth       = fifo_info.bitWidth;
       in.params.add.fifoWithDataType.version        = fifo_info.version;
                                                     //fifo_info.integerWordLength is not needed by the v1 kernel interface

       return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

   nirio_status niriok_proxy_impl_v1::set_device_config()
   {
      nirio_syncop_in_params_t in = {};
      nirio_syncop_out_params_t out = {};

      in.function    = NIRIO_FUNC::SET_DRIVER_CONFIG;
      in.subfunction = 0;

      return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::start_fifo(
       uint32_t channel)
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function    = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::START;

        in.params.fifo.channel = channel;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::stop_fifo(
        uint32_t channel)
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function    = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::STOP;

        in.params.fifo.channel = channel;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::configure_fifo(
       uint32_t channel,
       uint32_t requested_depth,
       uint8_t requires_actuals,
       uint32_t& actual_depth,
       uint32_t& actual_size)
    {
        nirio_status status = NiRio_Status_Success;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::CONFIGURE;

        in.params.fifo.channel = channel;
        in.params.fifo.op.config.requestedDepth = requested_depth;
        in.params.fifo.op.config.requiresActuals = requires_actuals;

        status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        if (nirio_status_fatal(status)) return status;

        actual_depth = out.params.fifo.op.config.actualDepth;
        actual_size = out.params.fifo.op.config.actualSize;

        return status;
    }

    nirio_status niriok_proxy_impl_v1::wait_on_fifo(
        uint32_t channel,
        uint32_t elements_requested,
        uint32_t scalar_type,
        uint32_t bit_width,
        uint32_t timeout,
        uint8_t output,
        void*& data_pointer,
        uint32_t& elements_acquired,
        uint32_t& elements_remaining)
    {
        nirio_status status = NiRio_Status_Success;

        nirio_syncop_in_params_t in = {};
        uint32_t stuffed[2];
        nirio_syncop_out_params_t out = {};
        init_syncop_out_params(out, stuffed, sizeof(stuffed));

        in.function    = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::WAIT;

        in.params.fifo.channel                   = channel;
        in.params.fifo.op.wait.elementsRequested = elements_requested;
        in.params.fifo.op.wait.scalarType        = scalar_type;
        in.params.fifo.op.wait.bitWidth          = bit_width;
        in.params.fifo.op.wait.output            = output;
        in.params.fifo.op.wait.timeout           = timeout;

        status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        if (nirio_status_fatal(status)) return status;

        data_pointer = out.params.fifo.op.wait.elements.pointer;
        elements_acquired = stuffed[0];
        elements_remaining = stuffed[1];

        return status;
    }

    nirio_status niriok_proxy_impl_v1::grant_fifo(
       uint32_t channel,
       uint32_t elements_to_grant)
    {
        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::GRANT;

        in.params.fifo.channel = channel;
        in.params.fifo.op.grant.elements = elements_to_grant;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy_impl_v1::read_fifo(
        uint32_t channel,
        uint32_t elements_to_read,
        void* buffer,
        uint32_t buffer_datatype_width,
        uint32_t scalar_type,
        uint32_t bit_width,
        uint32_t timeout,
        uint32_t& number_read,
        uint32_t& number_remaining)
    {
        nirio_status status = NiRio_Status_Success;

        nirio_syncop_in_params_t in = {};
        nirio_syncop_out_params_t out = {};
        init_syncop_out_params(out, buffer, elements_to_read * buffer_datatype_width);

        in.function = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::READ;

        in.params.fifo.channel = channel;
        in.params.fifo.op.readWithDataType.timeout = timeout;
        in.params.fifo.op.readWithDataType.scalarType = scalar_type;
        in.params.fifo.op.readWithDataType.bitWidth = bit_width;

        status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        if (nirio_status_fatal(status) && (status != NiRio_Status_FifoTimeout)) return status;

        number_read = out.params.fifo.op.read.numberRead;
        number_remaining = out.params.fifo.op.read.numberRemaining;

        return status;
    }

    nirio_status niriok_proxy_impl_v1::write_fifo(
        uint32_t channel,
        uint32_t elements_to_write,
        void* buffer,
        uint32_t buffer_datatype_width,
        uint32_t scalar_type,
        uint32_t bit_width,
        uint32_t timeout,
        uint32_t& number_remaining)
    {
        nirio_status status = NiRio_Status_Success;

        nirio_syncop_in_params_t in = {};
        init_syncop_in_params(in, buffer, elements_to_write * buffer_datatype_width);
        nirio_syncop_out_params_t out = {};

        in.function = NIRIO_FUNC::FIFO;
        in.subfunction = NIRIO_FIFO::WRITE;

        in.params.fifo.channel = channel;
        in.params.fifo.op.writeWithDataType.timeout = timeout;
        in.params.fifo.op.writeWithDataType.scalarType = scalar_type;
        in.params.fifo.op.writeWithDataType.bitWidth = bit_width;

        status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        if (nirio_status_fatal(status) && (status != NiRio_Status_FifoTimeout)) return status;

        number_remaining = out.params.fifo.op.write.numberRemaining;

        return status;
    }

}}

#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic pop
#endif
