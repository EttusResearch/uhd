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

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H

#include <stdint.h>
#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <uhd/transport/nirio/nirio_quirks.h>

namespace uhd { namespace niusrprio
{
    enum nirio_version_t { CURRENT, OLDEST_COMPATIBLE };

    enum nirio_device_attr_32_t {
       INTERFACE_NUMBER              =  1UL,
       PRODUCT_NUMBER                =  2UL,
       VENDOR_NUMBER                 =  3UL,
       SERIAL_NUMBER                 =  4UL,
       BUS_NUMBER                    = 10UL,
       DEVICE_NUMBER                 = 11UL,
       FUNCTION_NUMBER               = 12UL,
       CURRENT_VERSION               = 14UL,
       OLDEST_COMPATIBLE_VERSION     = 15UL,
       ADDRESS_SPACE                 = 25UL,
       IS_FPGA_PROGRAMMED            = 48UL,
       DEFAULT_FPGA_SIGNATURE_OFFSET = 53UL,
       DEFAULT_FPGA_RESET_OFFSET     = 54UL,
       DEFAULT_FPGA_RESET_SIZE       = 55UL,
       DEFAULT_FPGA_CONTROL_OFFSET   = 56UL,
       DEFAULT_FPGA_INTERRUPT_OFFSET = 57UL,
    };

    enum nirio_device_attr_str_t {
       PRODUCT_NAME                  = 0UL,
       FPGA_TARGET_CLASS             = 6UL,
       SAVED_BITFILE                 = 7UL,
    };

    enum nirio_addr_space_t {
       INVALID       = 0,
       BUS_INTERFACE = 1,
       FPGA          = 2,
       BAR_WINDOW    = 3,
    };


    class UHD_API niriok_proxy : public boost::noncopyable {
    public:
        typedef boost::shared_ptr<niriok_proxy> sptr;

        niriok_proxy();
        virtual ~niriok_proxy();

        //File operations
        nirio_status open(const std::string& interface_path);
        void close(void);

        nirio_status reset();

        inline uint32_t get_interface_num() { return _interface_num; }

        nirio_status get_cached_session(
            uint32_t& session);

        nirio_status get_version(
            nirio_version_t type,
            uint32_t& major,
            uint32_t& upgrade,
            uint32_t& maintenance,
            char& phase,
            uint32_t& build);

        nirio_status sync_operation(
            const void *writeBuffer,
            size_t writeBufferLength,
            void *readBuffer,
            size_t readBufferLength);

        nirio_status get_attribute(
            const nirio_device_attr_32_t attribute,
            uint32_t& attrValue);

        nirio_status get_attribute(
            const nirio_device_attr_str_t  attribute,
            char* buf,
            const uint32_t bufLen,
            uint32_t& stringLen);

        nirio_status set_attribute(
            const nirio_device_attr_32_t attribute,
            const uint32_t value);

        nirio_status set_attribute(
            const nirio_device_attr_str_t attribute,
            const char* const buffer);

        nirio_status peek(uint32_t offset, uint32_t& value);

        nirio_status peek(uint32_t offset, uint64_t& value);

        nirio_status poke(uint32_t offset, const uint32_t& value);

        nirio_status poke(uint32_t offset, const uint64_t& value);

        nirio_status map_fifo_memory(
            uint32_t fifo_instance,
            size_t size,
            nirio_driver_iface::rio_mmap_t& map);

        nirio_status unmap_fifo_memory(
            nirio_driver_iface::rio_mmap_t& map);

        nirio_status stop_all_fifos();

        nirio_quirks& get_rio_quirks() {
            return _rio_quirks;
        }

    private:    //Members
        nirio_driver_iface::rio_dev_handle_t    _device_handle;
        uint32_t                                _interface_num;
        nirio_quirks                            _rio_quirks;
    };

    class niriok_scoped_addr_space : public boost::noncopyable {
    public:
        explicit niriok_scoped_addr_space(niriok_proxy& proxy, nirio_addr_space_t addr_space, nirio_status& status) :
            driver_proxy(proxy)
        {
            cache_status = driver_proxy.get_attribute(ADDRESS_SPACE, cached_addr_space);
            nirio_status_chain(driver_proxy.set_attribute(ADDRESS_SPACE, addr_space), status);
        }

        ~niriok_scoped_addr_space() {
            if (nirio_status_not_fatal(cache_status))
                driver_proxy.set_attribute(ADDRESS_SPACE, cached_addr_space);
        }

    private:
        niriok_proxy& driver_proxy;
        uint32_t cached_addr_space;
        nirio_status cache_status;
    };
}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_INTERFACE_H */
