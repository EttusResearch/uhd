//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V2_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V2_H

#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <uhd/transport/nirio/nirio_quirks.h>
#include <uhd/transport/nirio/niriok_proxy.h>

#if __GNUC__
    typedef uint64_t  tAlignedU64 __attribute__ ((aligned(8)));
#else
    typedef uint64_t  tAlignedU64; 
#endif

namespace uhd { namespace niusrprio
{
   /* 
      This file defines the types, enumerations, and classes needed to 
      directly access a subset of the NI-RIO kernel interface.
      These definitions are specific to NI-RIO versions >= 14.0.  These 
      are not compatible with NI-RIO versions older than 14.0.
   */

   #define IOCTL(type, function, access) \
      CTL_CODE((0x8000+type), (0x800+function), METHOD_BUFFERED, access)

   #define IOCTL_ACCESS_ANY       (FILE_ANY_ACCESS)
   #define IOCTL_ACCESS_READ      (FILE_READ_ACCESS)
   #define IOCTL_ACCESS_WRITE     (FILE_WRITE_ACCESS)
   #define IOCTL_ACCESS_RW        (FILE_READ_ACCESS | FILE_WRITE_ACCESS)

   #define IOCTL_TRANSPORT_GET32 IOCTL(0, 0, IOCTL_ACCESS_READ)
   #define IOCTL_TRANSPORT_SET32 IOCTL(0, 1, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_GET_STRING IOCTL(0, 2, IOCTL_ACCESS_READ)
   #define IOCTL_TRANSPORT_SET_STRING IOCTL(0, 3, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_RESET IOCTL(1, 1, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_ADD_INPUT_FIFO_RESOURCE IOCTL(2, 0, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_ADD_OUTPUT_FIFO_RESOURCE IOCTL(2, 1, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_SET_DEVICE_CONFIG IOCTL(2, 3, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_FIFO_CONFIG IOCTL(4, 0, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_FIFO_START IOCTL(4, 1, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_FIFO_STOP IOCTL(4, 2, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_FIFO_READ IOCTL(4, 3, IOCTL_ACCESS_READ)
   #define IOCTL_TRANSPORT_FIFO_WRITE IOCTL(4, 4, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_FIFO_WAIT IOCTL(4, 5, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_FIFO_GRANT IOCTL(4, 6, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_FIFO_STOP_ALL IOCTL(4, 7, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_PEEK64 IOCTL(5, 2, IOCTL_ACCESS_READ)
   #define IOCTL_TRANSPORT_PEEK32 IOCTL(5, 3, IOCTL_ACCESS_READ)
   #define IOCTL_TRANSPORT_POKE64 IOCTL(5, 6, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_POKE32 IOCTL(5, 7, IOCTL_ACCESS_WRITE)
   #define IOCTL_TRANSPORT_POST_OPEN IOCTL(8, 0, IOCTL_ACCESS_ANY)
   #define IOCTL_TRANSPORT_PRE_CLOSE IOCTL(8, 1, IOCTL_ACCESS_ANY)

   typedef struct {
      nirio_scalar_type_t scalarType;
      nirio_u32_t     bitWidth;
      nirio_i32_t     integerWordLength;
   } nirio_fifo_data_type_t;

   class UHD_API niriok_proxy_impl_v2 : virtual public niriok_proxy {
    public:
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
		   tAlignedU64 requestedDepth;
		   int32_t status;
		} in_transport_fifo_config_t;
		typedef struct out_transport_fifo_config
		{
		   tAlignedU64  actualDepth;
		   tAlignedU64  actualSize;
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
		   tAlignedU64 buf;
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
		   tAlignedU64 buf;
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
		   tAlignedU64 elementsRequested;
		   nirio_fifo_data_type_t dataType;
		   bool output;
		   uint32_t timeout;
		   int32_t status;
		} in_transport_fifo_wait_t;
		typedef struct out_transport_fifo_wait
		{
		   tAlignedU64  elements;
		   tAlignedU64  elementsAcquired;
		   tAlignedU64  elementsRemaining;
		   int32_t status;
		} out_transport_fifo_wait_t;
		typedef struct in_transport_fifo_grant
		{
		   uint32_t channel;
		   tAlignedU64 elements;
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
		   tAlignedU64 retVal__;
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
		   tAlignedU64 value;
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

        niriok_proxy_impl_v2();
        virtual ~niriok_proxy_impl_v2();

        //File operations
        virtual nirio_status open(const std::string& interface_path);
        virtual void close(void);

        virtual nirio_status reset();

        virtual nirio_status get_cached_session(
            uint32_t& session);

        virtual nirio_status get_version(
            nirio_version_t type,
            uint32_t& major,
            uint32_t& upgrade,
            uint32_t& maintenance,
            char& phase,
            uint32_t& build);

        virtual nirio_status get_attribute(
            const nirio_device_attribute32_t attribute,
            uint32_t& attrValue);

        virtual nirio_status set_attribute(
            const nirio_device_attribute32_t attribute,
            const uint32_t value);

        virtual nirio_status peek(uint32_t offset, uint32_t& value);

        virtual nirio_status peek(uint32_t offset, uint64_t& value);

        virtual nirio_status poke(uint32_t offset, const uint32_t& value);

        virtual nirio_status poke(uint32_t offset, const uint64_t& value);

        virtual nirio_status map_fifo_memory(
            uint32_t fifo_instance,
            size_t size,
            nirio_driver_iface::rio_mmap_t& map);

        virtual nirio_status unmap_fifo_memory(
            nirio_driver_iface::rio_mmap_t& map);

        virtual nirio_status stop_all_fifos();

        virtual nirio_status add_fifo_resource(const nirio_fifo_info_t& fifo_info);

        virtual nirio_status set_device_config();

        virtual nirio_status start_fifo(
            uint32_t channel);

        virtual nirio_status stop_fifo(
            uint32_t channel);

        virtual nirio_status configure_fifo(
            uint32_t channel,
            uint32_t requested_depth,
            uint8_t requires_actuals,
            uint32_t& actual_depth,
            uint32_t& actual_size);

        virtual nirio_status wait_on_fifo(
            uint32_t channel,
            uint32_t elements_requested,
            uint32_t scalar_type,
            uint32_t bit_width,
            uint32_t timeout,
            uint8_t output,
            void*& data_pointer,
            uint32_t& elements_acquired,
            uint32_t& elements_remaining);

        virtual nirio_status grant_fifo(
            uint32_t channel,
            uint32_t elements_to_grant);

        virtual nirio_status read_fifo(
            uint32_t channel,
            uint32_t elements_to_read,
            void* buffer,
            uint32_t buffer_datatype_width,
            uint32_t scalar_type,
            uint32_t bit_width,
            uint32_t timeout,
            uint32_t& number_read,
            uint32_t& number_remaining);

        virtual nirio_status write_fifo(
            uint32_t channel,
            uint32_t elements_to_write,
            void* buffer,
            uint32_t buffer_datatype_width,
            uint32_t scalar_type,
            uint32_t bit_width,
            uint32_t timeout,
            uint32_t& number_remaining);

        protected:
            // protected close function that doesn't acquire synchronization lock
            virtual void _close();

    };

}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V2_H */
