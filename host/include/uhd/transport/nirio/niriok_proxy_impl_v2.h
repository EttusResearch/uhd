//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V2_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_PROXY_IMPL_V2_H

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
      These definitions are specific to NI-RIO versions >= 14.0.  These 
      are not compatible with NI-RIO versions older than 14.0.
   */

   class UHD_API niriok_proxy_impl_v2 : virtual public niriok_proxy {
    public:
        niriok_proxy_impl_v2();
        virtual ~niriok_proxy_impl_v2();

        //File operations
        virtual nirio_status open(const std::string& interface_path);
        virtual void close(void);

        virtual nirio_status reset();

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
