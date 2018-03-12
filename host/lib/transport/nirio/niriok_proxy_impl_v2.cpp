//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/transport/nirio/niriok_proxy_impl_v2.h>
#include <cstring>

// "push" and "pop" introduced in GCC 4.6; works with all clang
#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define IOCTL_TRANSPORT_GET32                       IOCTL(0, 0, IOCTL_ACCESS_READ)
#define IOCTL_TRANSPORT_SET32                       IOCTL(0, 1, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_GET_STRING                  IOCTL(0, 2, IOCTL_ACCESS_READ)
#define IOCTL_TRANSPORT_SET_STRING                  IOCTL(0, 3, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_RESET                       IOCTL(1, 1, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_ADD_INPUT_FIFO_RESOURCE     IOCTL(2, 0, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_ADD_OUTPUT_FIFO_RESOURCE    IOCTL(2, 1, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_SET_DEVICE_CONFIG           IOCTL(2, 3, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_FIFO_CONFIG                 IOCTL(4, 0, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_FIFO_START                  IOCTL(4, 1, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_FIFO_STOP                   IOCTL(4, 2, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_FIFO_READ                   IOCTL(4, 3, IOCTL_ACCESS_READ)
#define IOCTL_TRANSPORT_FIFO_WRITE                  IOCTL(4, 4, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_FIFO_WAIT                   IOCTL(4, 5, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_FIFO_GRANT                  IOCTL(4, 6, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_FIFO_STOP_ALL               IOCTL(4, 7, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_PEEK64                      IOCTL(5, 2, IOCTL_ACCESS_READ)
#define IOCTL_TRANSPORT_PEEK32                      IOCTL(5, 3, IOCTL_ACCESS_READ)
#define IOCTL_TRANSPORT_POKE64                      IOCTL(5, 6, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_POKE32                      IOCTL(5, 7, IOCTL_ACCESS_WRITE)
#define IOCTL_TRANSPORT_POST_OPEN                   IOCTL(8, 0, IOCTL_ACCESS_ANY)
#define IOCTL_TRANSPORT_PRE_CLOSE                   IOCTL(8, 1, IOCTL_ACCESS_ANY)

namespace uhd { namespace niusrprio
{
    //-------------------------------------------------------
    // ioctl param typedefs
    //-------------------------------------------------------
    typedef struct {
       nirio_scalar_type_t scalarType;
       nirio_u32_t     bitWidth;
       nirio_i32_t     integerWordLength;
    } nirio_fifo_data_type_t;

    typedef struct in_transport_get32
    {
       nirio_device_attribute32_t attribute;
       int32_t status;
    } in_transport_get32_t;
    typedef struct out_transport_get32
    {
       uint32_t retVal__;
       int32_t status;
    } out_transport_get32_t;
    typedef struct in_transport_set32
    {
       nirio_device_attribute32_t attribute;
       uint32_t value;
       int32_t status;
    } in_transport_set32_t;
    typedef struct out_transport_set32
    {
       int32_t status;
    } out_transport_set32_t;
    typedef struct out_transport_get_string
    {
       uint32_t  stringLen;
       int32_t status;
    } out_transport_get_string_t;
    typedef struct out_transport_set_string
    {
       int32_t status;
    } out_transport_set_string_t;
    typedef struct in_transport_reset
    {
       int32_t status;
    } in_transport_reset_t;
    typedef struct out_transport_reset
    {
       int32_t status;
    } out_transport_reset_t;
    typedef struct in_transport_add_input_fifo_resource
    {
       uint32_t channel;
       uint32_t baseAddress;
       uint32_t depthInSamples;
       nirio_fifo_data_type_t dataType;
       uint32_t version;
       int32_t status;
    } in_transport_add_input_fifo_resource_t;
    typedef struct out_transport_addInputFifo_resource
    {
       int32_t status;
    } out_transport_add_input_fifo_resource_t;
    typedef struct in_transport_addOutputFifo_resource
    {
       uint32_t channel;
       uint32_t baseAddress;
       uint32_t depthInSamples;
       nirio_fifo_data_type_t dataType;
       uint32_t version;
       int32_t status;
    } in_transport_add_output_fifo_resource_t;
    typedef struct out_transport_addOutputFifo_resource
    {
       int32_t status;
    } out_transport_add_output_fifo_resource_t;
    typedef struct in_transport_setDevice_config
    {
       uint32_t attribute;
       int32_t status;
    } in_transport_set_device_config_t;
    typedef struct out_transport_setDevice_config
    {
       int32_t status;
    } out_transport_set_device_config_t;
    typedef struct in_transport_fifo_config
    {
       uint32_t channel;
       aligned_uint64_t requestedDepth;
       int32_t status;
    } in_transport_fifo_config_t;
    typedef struct out_transport_fifo_config
    {
       aligned_uint64_t  actualDepth;
       aligned_uint64_t  actualSize;
       int32_t status;
    } out_transport_fifo_config_t;
    typedef struct in_transport_fifo_start
    {
       uint32_t channel;
       int32_t status;
    } in_transport_fifo_start_t;
    typedef struct out_transport_fifo_start
    {
       int32_t status;
    } out_transport_fifo_start_t;
    typedef struct in_transport_fifo_stop
    {
       uint32_t channel;
       int32_t status;
    } in_transport_fifo_stop_t;
    typedef struct out_transport_fifo_stop
    {
       int32_t status;
    } out_transport_fifo_stop_t;
    typedef struct in_transport_fifo_read
    {
       uint32_t channel;
       aligned_uint64_t buf;
       uint32_t numberElements;
       nirio_fifo_data_type_t dataType;
       uint32_t timeout;
       int32_t status;
    } in_transport_fifo_read_t;
    typedef struct out_transport_fifo_read
    {
       uint32_t  read;
       uint32_t  remaining;
       int32_t status;
    } out_transport_fifo_read_t;
    typedef struct in_transport_fifo_write
    {
       uint32_t channel;
       aligned_uint64_t buf;
       uint32_t numberElements;
       nirio_fifo_data_type_t dataType;
       uint32_t timeout;
       int32_t status;
    } in_transport_fifo_write_t;
    typedef struct out_transport_fifo_write
    {
       uint32_t  remaining;
       int32_t status;
    } out_transport_fifo_write_t;
    typedef struct in_transport_fifo_wait
    {
       uint32_t channel;
       aligned_uint64_t elementsRequested;
       nirio_fifo_data_type_t dataType;
       bool output;
       uint32_t timeout;
       int32_t status;
    } in_transport_fifo_wait_t;
    typedef struct out_transport_fifo_wait
    {
       aligned_uint64_t  elements;
       aligned_uint64_t  elementsAcquired;
       aligned_uint64_t  elementsRemaining;
       int32_t status;
    } out_transport_fifo_wait_t;
    typedef struct in_transport_fifo_grant
    {
       uint32_t channel;
       aligned_uint64_t elements;
       int32_t status;
    } in_transport_fifo_grant_t;
    typedef struct out_transport_fifo_grant
    {
       int32_t status;
    } out_transport_fifo_grant_t;
    typedef struct in_transport_fifoStop_all
    {
       int32_t status;
    } in_transport_fifo_stop_all_t;
    typedef struct out_transport_fifoStop_all
    {
       int32_t status;
    } out_transport_fifo_stop_all_t;
    typedef struct in_transport_peek64
    {
       uint32_t offset;
       int32_t status;
    } in_transport_peek64_t;
    typedef struct out_transport_peek64
    {
       aligned_uint64_t retVal__;
       int32_t status;
    } out_transport_peek64_t;
    typedef struct in_transport_peek32
    {
       uint32_t offset;
       int32_t status;
    } in_transport_peek32_t;
    typedef struct out_transport_peek32
    {
       uint32_t retVal__;
       int32_t status;
    } out_transport_peek32_t;
    typedef struct in_transport_poke64
    {
       uint32_t offset;
       aligned_uint64_t value;
       int32_t status;
    } in_transport_poke64_t;
    typedef struct out_transport_poke64
    {
       int32_t status;
    } out_transport_poke64_t;
    typedef struct in_transport_poke32
    {
       uint32_t offset;
       uint32_t value;
       int32_t status;
    } in_transport_poke32_t;
    typedef struct out_transport_poke32
    {
       int32_t status;
    } out_transport_poke32_t;
    typedef struct in_transport_post_open
    {
       int32_t status;
    } in_transport_post_open_t;
    typedef struct out_transport_post_open
    {
       int32_t status;
    } out_transport_post_open_t;
    typedef struct in_transport_pre_close
    {
       int32_t status;
    } in_transport_pre_close_t;
    typedef struct out_transport_pre_close
    {
       int32_t status;
    } out_transport_pre_close_t;

    //-------------------------------------------------------
    // niriok_proxy_impl_v2
    //-------------------------------------------------------
    niriok_proxy_impl_v2::niriok_proxy_impl_v2()
    {
    }

    niriok_proxy_impl_v2::~niriok_proxy_impl_v2()
    {
        close();
    }

    nirio_status niriok_proxy_impl_v2::open(const std::string& interface_path)
    {
        WRITER_LOCK

        if (interface_path.empty()) return NiRio_Status_ResourceNotFound;

        //close if already open.
        // use non-locking _close since we already have the lock
        _close();

        in_transport_post_open_t in = {};
        out_transport_post_open_t out = {};

        in.status = NiRio_Status_Success;

        nirio_status status = NiRio_Status_Success;
        nirio_status_chain(nirio_driver_iface::rio_open(
        interface_path, _device_handle), status);
        if (nirio_status_not_fatal(status)) 
        {
            nirio_status_chain(nirio_driver_iface::rio_ioctl(_device_handle,
                                        IOCTL_TRANSPORT_POST_OPEN,
                                        &in, sizeof(in), &out, sizeof(out)), status);
            if (nirio_status_fatal(status)) _close();
        }
        return status;
    }

    void niriok_proxy_impl_v2::close(void)
    {
       WRITER_LOCK

       _close();
    }

    // this protected _close doesn't acquire the lock, so it can be used in methods 
    // that already have the lock
    void niriok_proxy_impl_v2::_close()
    {
        if(nirio_driver_iface::rio_isopen(_device_handle))
        {
            in_transport_pre_close_t in = {};
            out_transport_pre_close_t out = {};

            in.status = NiRio_Status_Success;

            nirio_driver_iface::rio_ioctl(
            _device_handle, IOCTL_TRANSPORT_PRE_CLOSE, &in, sizeof(in), &out, sizeof(out));
            nirio_driver_iface::rio_close(_device_handle);
       }
    }

    nirio_status niriok_proxy_impl_v2::reset()
    {
        READER_LOCK

        in_transport_reset_t in = {};
        out_transport_reset_t out = {};

        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_RESET,
                                    &in, sizeof(in), &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::get_version(
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

    nirio_status niriok_proxy_impl_v2::get_attribute(
        const nirio_device_attribute32_t attribute,
        uint32_t& attrValue)
    {
        READER_LOCK

        in_transport_get32_t in = {};
        out_transport_get32_t out = {};

        in.attribute = attribute;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                 IOCTL_TRANSPORT_GET32,
                                 &in, sizeof(in),
                                 &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        attrValue = out.retVal__;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::set_attribute(
        const nirio_device_attribute32_t attribute,
        const uint32_t value)
    {
        READER_LOCK

        in_transport_set32_t in = {};
        out_transport_set32_t out = {};

        in.attribute = attribute;
        in.value  = value;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                 IOCTL_TRANSPORT_SET32,
                                 &in, sizeof(in),
                                 &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::peek(uint32_t offset, uint32_t& value)
    {
        READER_LOCK

        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;
        
        in_transport_peek32_t in = {};
        out_transport_peek32_t out = {};
                
        in.offset = offset;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_PEEK32,
                                    &in, sizeof(in),
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        value = out.retVal__;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::peek(uint32_t offset, uint64_t& value)
    {
        READER_LOCK

        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;
        in_transport_peek64_t in = {};
        out_transport_peek64_t out = {};

        in.offset = offset;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_PEEK64,
                                    &in, sizeof(in),
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        value = out.retVal__;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::poke(uint32_t offset, const uint32_t& value)
    {
        READER_LOCK

        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;

        in_transport_poke32_t in = {};
        out_transport_poke32_t out = {};

        in.offset = offset;
        in.value = value;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_POKE32,
                                    &in, sizeof(in),
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::poke(uint32_t offset, const uint64_t& value)
    {
        READER_LOCK

        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;

        in_transport_poke64_t in = {};
        out_transport_poke64_t out = {};

        in.offset = offset;
        in.value = value;
        in.status = NiRio_Status_Success;

        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_POKE64,
                                    &in, sizeof(in),
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::map_fifo_memory(
        uint32_t fifo_instance,
        size_t size,
        nirio_driver_iface::rio_mmap_t& map)
    {
        READER_LOCK

        return nirio_driver_iface::rio_mmap(_device_handle,
                GET_FIFO_MEMORY_TYPE(fifo_instance),
                size, true, map);
    }

    nirio_status niriok_proxy_impl_v2::unmap_fifo_memory(
        nirio_driver_iface::rio_mmap_t& map)
    {
        READER_LOCK

        return nirio_driver_iface::rio_munmap(map);
    }

    nirio_status niriok_proxy_impl_v2::stop_all_fifos()
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_stop_all_t in = {};
        out_transport_fifo_stop_all_t out = {};

        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_STOP_ALL,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

   nirio_status niriok_proxy_impl_v2::add_fifo_resource(const nirio_fifo_info_t& fifo_info)
   {
      READER_LOCK

      nirio_status status = NiRio_Status_Success;
      nirio_status ioctl_status = NiRio_Status_Success;

      switch(fifo_info.direction)
      {
         case INPUT_FIFO:
         {
            in_transport_add_input_fifo_resource_t in = {};
            out_transport_add_input_fifo_resource_t out = {};

            in.channel = fifo_info.channel;
            in.baseAddress = fifo_info.base_addr;
            in.depthInSamples = fifo_info.depth;
            in.dataType.scalarType = fifo_info.scalar_type;
            in.dataType.bitWidth = fifo_info.bitWidth;
            in.dataType.integerWordLength = fifo_info.integerWordLength; 
            in.version = fifo_info.version;
            in.status = NiRio_Status_Success;

            ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_ADD_INPUT_FIFO_RESOURCE,
                                    &in, sizeof(in),
                                    &out, sizeof(out));

            status = nirio_status_fatal(ioctl_status) ? ioctl_status : out.status;
            break;
         }
         case OUTPUT_FIFO:
         {
            in_transport_add_output_fifo_resource_t in = {};
            out_transport_add_output_fifo_resource_t out = {};

            in.channel = fifo_info.channel;
            in.baseAddress = fifo_info.base_addr;
            in.depthInSamples = fifo_info.depth;
            in.dataType.scalarType = fifo_info.scalar_type;
            in.dataType.bitWidth = fifo_info.bitWidth;
            in.dataType.integerWordLength = fifo_info.integerWordLength;
            in.version = fifo_info.version;
            in.status = NiRio_Status_Success;
           
            ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    IOCTL_TRANSPORT_ADD_OUTPUT_FIFO_RESOURCE,
                                    &in, sizeof(in),
                                    &out, sizeof(out));

            status = nirio_status_fatal(ioctl_status) ? ioctl_status : out.status;
            break;
         }
         default:
            status = NiRio_Status_SoftwareFault;
         }
         
         return status;
    }

   nirio_status niriok_proxy_impl_v2::set_device_config()
   {
      READER_LOCK

      nirio_status ioctl_status = NiRio_Status_Success;

      in_transport_set_device_config_t in = {};
      out_transport_set_device_config_t out = {};

      in.attribute = 0;  //this is unused in the kernel
      in.status = NiRio_Status_Success;

      ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                              IOCTL_TRANSPORT_SET_DEVICE_CONFIG,
                              &in, sizeof(in),
                              &out, sizeof(out));

      return nirio_status_fatal(ioctl_status) ? ioctl_status : out.status;
   }

    nirio_status niriok_proxy_impl_v2::start_fifo(
       uint32_t channel)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_start_t in = {};
        out_transport_fifo_start_t out = {};

        in.channel = channel;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_START,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::stop_fifo(
        uint32_t channel)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_stop_t in = {};
        out_transport_fifo_stop_t out = {};

        in.channel = channel;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_STOP,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::configure_fifo(
       uint32_t channel,
       uint32_t requested_depth,
       uint8_t /*requires_actuals*/, //Unused
       uint32_t& actual_depth,
       uint32_t& actual_size)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_config_t in = {};
        out_transport_fifo_config_t out = {};

        in.channel = channel;
        in.requestedDepth = requested_depth;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_CONFIG,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        UHD_ASSERT_THROW(out.actualDepth <= std::numeric_limits<uint32_t>::max());
        actual_depth = static_cast<uint32_t>(out.actualDepth);
        UHD_ASSERT_THROW(out.actualSize <= std::numeric_limits<uint32_t>::max());
        actual_size = static_cast<uint32_t>(out.actualSize);
        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::wait_on_fifo(
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
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_wait_t in = {};
        out_transport_fifo_wait_t out = {};

        in.channel = channel;
        in.elementsRequested = elements_requested;
        in.dataType.scalarType = map_int_to_scalar_type(scalar_type);
        in.dataType.bitWidth = bit_width;
        in.dataType.integerWordLength = bit_width; // same as bit_width for all types except fixed point, which is not supported
        in.output = (output != 0);
        in.timeout = timeout;
        in.status = NiRio_Status_Success;

           ioctl_status =
           nirio_driver_iface::rio_ioctl(
              _device_handle,
              IOCTL_TRANSPORT_FIFO_WAIT,
              &in,
              sizeof(in),
              &out,
              sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        data_pointer = reinterpret_cast<void*>(out.elements);
        UHD_ASSERT_THROW(out.elementsAcquired <= std::numeric_limits<uint32_t>::max());
        elements_acquired = static_cast<uint32_t>(out.elementsAcquired);
        UHD_ASSERT_THROW(out.elementsRemaining <= std::numeric_limits<uint32_t>::max());
        elements_remaining = static_cast<uint32_t>(out.elementsRemaining);
        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::grant_fifo(
       uint32_t channel,
       uint32_t elements_to_grant)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_grant_t in = {};
        out_transport_fifo_grant_t out = {};

        in.channel = channel;
        in.elements = elements_to_grant;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_GRANT,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::read_fifo(
        uint32_t channel,
        uint32_t elements_to_read,
        void* buffer,
        uint32_t /*buffer_datatype_width*/, //Unused
        uint32_t scalar_type,
        uint32_t bit_width,
        uint32_t timeout,
        uint32_t& number_read,
        uint32_t& number_remaining)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_read_t in = {};
        out_transport_fifo_read_t out = {};

        in.channel = channel;
        in.buf = reinterpret_cast<aligned_uint64_t>(buffer);
        in.numberElements = elements_to_read;
        in.dataType.scalarType = map_int_to_scalar_type(scalar_type);
        in.dataType.bitWidth = bit_width;
        in.dataType.integerWordLength = bit_width; // same as bit_width for all types except fixed point, which is not supported
        in.timeout = timeout;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_READ,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        number_read = out.read;
        number_remaining = out.remaining;
        return out.status;
    }

    nirio_status niriok_proxy_impl_v2::write_fifo(
        uint32_t channel,
        uint32_t elements_to_write,
        void* buffer,
        uint32_t /*buffer_datatype_width*/, //Unused
        uint32_t scalar_type,
        uint32_t bit_width,
        uint32_t timeout,
        uint32_t& number_remaining)
    {
        READER_LOCK

        nirio_status ioctl_status = NiRio_Status_Success;
        in_transport_fifo_write_t in = {};
        out_transport_fifo_write_t out = {};

        in.channel = channel;
        in.buf = reinterpret_cast<aligned_uint64_t>(buffer);
        in.numberElements = elements_to_write;
        in.dataType.scalarType = map_int_to_scalar_type(scalar_type);
        in.dataType.bitWidth = bit_width;
        in.dataType.integerWordLength = bit_width; // same as bit_width for all types except fixed point, which is not supported
        in.timeout = timeout;
        in.status = NiRio_Status_Success;

        ioctl_status = 
           nirio_driver_iface::rio_ioctl(
               _device_handle,
               IOCTL_TRANSPORT_FIFO_WRITE,
               &in,
               sizeof(in),
               &out,
               sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        number_remaining = out.remaining;
        return out.status;
    }

}}

#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic pop
#endif
