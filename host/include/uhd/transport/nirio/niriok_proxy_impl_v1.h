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

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V1_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V1_H

#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <uhd/transport/nirio/nirio_quirks.h>
#include <uhd/transport/nirio/niriok_proxy.h>

namespace uhd { namespace niusrprio
{
   /* 
      This file defines the types, enumerations, and classes needed to 
      directly access a subset of the NI-RIO kernel interface.
      These definitions are specific to NI-RIO versions < 14.0.  These 
      are not compatible with NI-RIO 14.0 and later.
   */

   class UHD_API niriok_proxy_impl_v1 : virtual public niriok_proxy {
    public:

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

        niriok_proxy_impl_v1();
        virtual ~niriok_proxy_impl_v1();

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

        private:
            nirio_status sync_operation(
                const void *writeBuffer,
                size_t writeBufferLength,
                void *readBuffer,
                size_t readBufferLength);

    };

}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V1_H */
