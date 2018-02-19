//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H

#include <stdint.h>
#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <uhd/transport/nirio/nirio_quirks.h>

#define NI_VENDOR_NUM   0x1093

#define VERSION_BUILD_SHIFT     0
#define VERSION_PHASE_SHIFT     14
#define VERSION_MAINT_SHIFT     16
#define VERSION_UPGRD_SHIFT     20
#define VERSION_MAJOR_SHIFT     24
#define VERSION_BUILD_MASK      0x00003FFF
#define VERSION_PHASE_MASK      0x0000C000
#define VERSION_MAINT_MASK      0x000F0000
#define VERSION_UPGRD_MASK      0x00F00000
#define VERSION_MAJOR_MASK      0xFF000000

#define GET_FIFO_MEMORY_TYPE(fifo_inst) (static_cast<uint16_t>(0x0100 | static_cast<uint16_t>(fifo_inst)))

#define READER_LOCK \
    boost::shared_lock<boost::shared_mutex> reader_lock(_synchronization);

#define WRITER_LOCK \
    boost::upgrade_lock<boost::shared_mutex> write_upgrade_lock(_synchronization);\
    boost::upgrade_to_unique_lock<boost::shared_mutex> write_unique_lock(write_upgrade_lock);


namespace uhd { namespace niusrprio
{
    enum nirio_version_t { CURRENT, OLDEST_COMPATIBLE };

    enum nirio_addr_space_t {
       INVALID       = 0,
       BUS_INTERFACE = 1,
       FPGA          = 2,
       BAR_WINDOW    = 3,
    };

    typedef uint64_t nirio_u64_t;
    typedef uint32_t nirio_u32_t;
    typedef uint16_t nirio_u16_t;
    typedef uint8_t nirio_u8_t;
    typedef int32_t nirio_i32_t;

    typedef enum {
        RIO_PRODUCT_NUMBER                  =  2UL, // 200
        RIO_CURRENT_VERSION                 = 14UL, // 220
        RIO_OLDEST_COMPATIBLE_VERSION       = 15UL, // 220
        RIO_ADDRESS_SPACE                   = 25UL, // 230
        RIO_IS_FPGA_PROGRAMMED              = 48UL, // 300
        RIO_FPGA_DEFAULT_SIGNATURE_OFFSET   = 53UL, // 300 Default Offsets for FPGA
                                               //     registers. Supplied by
                                               //     the board driver on device
                                               //     start.
    } nirio_device_attribute32_t;

   typedef enum {
      RIO_SCALAR_TYPE_IB = 1UL,
      RIO_SCALAR_TYPE_IW = 2UL,
      RIO_SCALAR_TYPE_IL = 3UL,
      RIO_SCALAR_TYPE_IQ = 4UL,
      RIO_SCALAR_TYPE_UB = 5UL,
      RIO_SCALAR_TYPE_UW = 6UL,
      RIO_SCALAR_TYPE_UL = 7UL,
      RIO_SCALAR_TYPE_UQ = 8UL,
   } nirio_scalar_type_t;
   
   static inline nirio_scalar_type_t map_int_to_scalar_type(uint32_t scalar_type_as_int)
   {
      switch (scalar_type_as_int)
      {
      case 1: return RIO_SCALAR_TYPE_IB;
      case 2: return RIO_SCALAR_TYPE_IW;
      case 3: return RIO_SCALAR_TYPE_IL;
      case 4: return RIO_SCALAR_TYPE_IQ;
      case 5: return RIO_SCALAR_TYPE_UB;
      case 6: return RIO_SCALAR_TYPE_UW;
      case 7: return RIO_SCALAR_TYPE_UL;
      case 8: return RIO_SCALAR_TYPE_UQ;
      default: UHD_ASSERT_THROW(false); return RIO_SCALAR_TYPE_UL;
      }
   }
   
   enum fifo_direction_t {
       INPUT_FIFO,
       OUTPUT_FIFO
   };

   struct nirio_fifo_info_t {
       nirio_fifo_info_t(
           uint32_t             arg_channel,
           const char*         arg_name,
           fifo_direction_t    arg_direction,
           uint32_t             arg_base_addr,
           uint32_t             arg_depth,
           nirio_scalar_type_t         arg_scalar_type,
           uint32_t            arg_bitWidth,
           int32_t               arg_integerWordLength,
           uint32_t             arg_version) :
               channel(arg_channel),
               name(arg_name),
               direction(arg_direction),
               base_addr(arg_base_addr),
               depth(arg_depth),
               scalar_type(arg_scalar_type),
               bitWidth(arg_bitWidth),
               integerWordLength(arg_integerWordLength),
               version(arg_version)
       {}

       uint32_t             channel;
       std::string            name;
       fifo_direction_t    direction;
       uint32_t             base_addr;
       uint32_t             depth;
       nirio_scalar_type_t         scalar_type;
       uint32_t            bitWidth;
       int32_t             integerWordLength;
       uint32_t             version;
   };

    class UHD_API niriok_proxy : public boost::noncopyable {
    public:
        typedef boost::shared_ptr<niriok_proxy> sptr;

        static sptr make_and_open(const std::string& interface_path);

        niriok_proxy();
        virtual ~niriok_proxy();

        //File operations
        virtual nirio_status open(const std::string& interface_path) = 0;
        virtual void close(void) = 0;

        virtual nirio_status reset() = 0;

        uint32_t get_interface_num() { return _interface_num; }

        virtual nirio_status get_version(
            nirio_version_t type,
            uint32_t& major,
            uint32_t& upgrade,
            uint32_t& maintenance,
            char& phase,
            uint32_t& build) = 0;

        virtual nirio_status get_attribute(
            const nirio_device_attribute32_t attribute,
            uint32_t& attrValue) = 0;

        virtual nirio_status set_attribute(
            const nirio_device_attribute32_t attribute,
            const uint32_t value) = 0;

        virtual nirio_status peek(uint32_t offset, uint32_t& value) = 0;

        virtual nirio_status peek(uint32_t offset, uint64_t& value) = 0;

        virtual nirio_status poke(uint32_t offset, const uint32_t& value) = 0;

        virtual nirio_status poke(uint32_t offset, const uint64_t& value) = 0;

        virtual nirio_status map_fifo_memory(
            uint32_t fifo_instance,
            size_t size,
            nirio_driver_iface::rio_mmap_t& map) = 0;

        virtual nirio_status unmap_fifo_memory(
            nirio_driver_iface::rio_mmap_t& map) = 0;

        virtual nirio_status stop_all_fifos() = 0;

        nirio_quirks& get_rio_quirks() {
            return _rio_quirks;
        }

        virtual nirio_status add_fifo_resource(const nirio_fifo_info_t& fifo_info) = 0;

        virtual nirio_status set_device_config() = 0;

        virtual nirio_status start_fifo(
           uint32_t channel) = 0;

        virtual nirio_status stop_fifo(
           uint32_t channel) = 0;

        virtual nirio_status configure_fifo(
           uint32_t channel,
           uint32_t requested_depth,
           uint8_t requires_actuals,
           uint32_t& actual_depth,
           uint32_t& actual_size) = 0;

        virtual nirio_status wait_on_fifo(
           uint32_t channel,
           uint32_t elements_requested,
           uint32_t scalar_type,
           uint32_t bit_width,
           uint32_t timeout,
           uint8_t output,
           void*& data_pointer,
           uint32_t& elements_acquired,
           uint32_t& elements_remaining) = 0;

        virtual nirio_status grant_fifo(
           uint32_t channel,
           uint32_t elements_to_grant) = 0;

        virtual nirio_status read_fifo(
           uint32_t channel,
           uint32_t elements_to_read,
           void* buffer,
           uint32_t buffer_datatype_width,
           uint32_t scalar_type,
           uint32_t bit_width,
           uint32_t timeout,
           uint32_t& number_read,
           uint32_t& number_remaining) = 0;

        virtual nirio_status write_fifo(
           uint32_t channel,
           uint32_t elements_to_write,
           void* buffer,
           uint32_t buffer_datatype_width,
           uint32_t scalar_type,
           uint32_t bit_width,
           uint32_t timeout,
           uint32_t& number_remaining) = 0;

    protected:    //Members
        nirio_driver_iface::rio_dev_handle_t    _device_handle;
        uint32_t                                _interface_num;
        nirio_quirks                            _rio_quirks;

        static boost::shared_mutex              _synchronization;

        // protected close function that doesn't acquire synchronization lock
        virtual void _close() = 0;
    };

    class niriok_scoped_addr_space : public boost::noncopyable {
    public:
        explicit niriok_scoped_addr_space(niriok_proxy::sptr proxy, nirio_addr_space_t addr_space, nirio_status& status) :
            driver_proxy(proxy)
        {
            cache_status = driver_proxy->get_attribute(RIO_ADDRESS_SPACE, cached_addr_space);
            nirio_status_chain(driver_proxy->set_attribute(RIO_ADDRESS_SPACE, addr_space), status);
        }

        ~niriok_scoped_addr_space() {
            if (nirio_status_not_fatal(cache_status))
                driver_proxy->set_attribute(RIO_ADDRESS_SPACE, cached_addr_space);
        }

    private:
        niriok_proxy::sptr driver_proxy;
        uint32_t cached_addr_space;
        nirio_status cache_status;
    };
}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H */
