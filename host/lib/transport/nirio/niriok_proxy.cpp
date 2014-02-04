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


#include <uhd/transport/nirio/niriok_proxy.h>
#include <cstring>

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

#ifdef __clang__
    #pragma GCC diagnostic push ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace uhd { namespace niusrprio
{
    //-------------------------------------------------------
    // niriok_proxy
    //-------------------------------------------------------
    niriok_proxy::niriok_proxy(): _device_handle(nirio_driver_iface::INVALID_RIO_HANDLE)
    {
    }

    niriok_proxy::~niriok_proxy()
    {
        close();
    }

    nirio_status niriok_proxy::open(const std::string& interface_path)
    {
        if (interface_path.empty()) return NiRio_Status_ResourceNotFound;

        //close if already open.
        close();

        nirio_status status = NiRio_Status_Success;
        nirio_status_chain(nirio_driver_iface::rio_open(
            interface_path, _device_handle), status);
        if (nirio_status_not_fatal(status)) {
            nirio_status_chain(nirio_driver_iface::rio_ioctl(_device_handle,
                                        nirio_driver_iface::NIRIO_IOCTL_POST_OPEN,
                                        NULL, 0, NULL, 0), status);
            nirio_driver_iface::nirio_ioctl_packet_t out(&_interface_num, sizeof(_interface_num), 0);
            nirio_status_chain(nirio_driver_iface::rio_ioctl(_device_handle,
                                        nirio_driver_iface::NIRIO_IOCTL_GET_IFACE_NUM,
                                        NULL, 0,
                                        &out, sizeof(out)), status);

            if (nirio_status_fatal(status)) close();
        }
        return status;
    }

    void niriok_proxy::close(void)
    {
       if(nirio_driver_iface::rio_isopen(_device_handle))
       {
            nirio_driver_iface::rio_ioctl(
                _device_handle, nirio_driver_iface::NIRIO_IOCTL_PRE_CLOSE, NULL, 0, NULL, 0);
            nirio_driver_iface::rio_close(_device_handle);
       }
    }

    nirio_status niriok_proxy::reset()
    {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function = nirio_driver_iface::NIRIO_FUNC::RESET;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy::get_cached_session(
        uint32_t& session)
    {
        nirio_driver_iface::nirio_ioctl_packet_t out(&session, sizeof(session), 0);
        return nirio_driver_iface::rio_ioctl(_device_handle,
                                    nirio_driver_iface::NIRIO_IOCTL_GET_SESSION,
                                    NULL, 0,
                                    &out, sizeof(out));
    }

    nirio_status niriok_proxy::get_version(
        nirio_version_t type,
        uint32_t& major,
        uint32_t& upgrade,
        uint32_t& maintenance,
        char& phase,
        uint32_t& build)
    {
        nirio_device_attr_32_t version_attr = (type==CURRENT)?CURRENT_VERSION:OLDEST_COMPATIBLE_VERSION;
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

    nirio_status niriok_proxy::sync_operation(
        const void *writeBuffer,
        size_t writeBufferLength,
        void *readBuffer,
        size_t readBufferLength)
    {
        nirio_driver_iface::nirio_ioctl_packet_t out(readBuffer, readBufferLength, 0);
        nirio_status ioctl_status = nirio_driver_iface::rio_ioctl(_device_handle,
                                    nirio_driver_iface::NIRIO_IOCTL_SYNCOP,
                                    writeBuffer, writeBufferLength,
                                    &out, sizeof(out));
        if (nirio_status_fatal(ioctl_status)) return ioctl_status;

        return out.statusCode;
    }

    nirio_status niriok_proxy::get_attribute(
        const nirio_device_attr_32_t attribute,
        uint32_t& attrValue)
    {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function = nirio_driver_iface::NIRIO_FUNC::GET32;
        in.params.attribute32.attribute = attribute;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));

        attrValue = out.params.attribute32.value;
        return status;
    }

    nirio_status niriok_proxy::get_attribute(
        const nirio_device_attr_str_t  attribute,
        char *buf,
        const uint32_t bufLen,
        uint32_t& stringLen)
    {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};
        nirio_driver_iface::init_syncop_out_params(out, buf, bufLen);

        in.function = nirio_driver_iface::NIRIO_FUNC::GET_STRING;
        in.params.attributeStr.attribute = attribute;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));

        stringLen = out.params.stringLength;
        return status;
    }

    nirio_status niriok_proxy::set_attribute(
        const nirio_device_attr_32_t attribute,
        const uint32_t value)
    {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

       in.function = nirio_driver_iface::NIRIO_FUNC::SET32;
       in.params.attribute32.attribute = attribute;
       in.params.attribute32.value  = value;

       return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy::set_attribute(
        const nirio_device_attr_str_t attribute,
        const char* const buffer)
    {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::init_syncop_in_params(in, buffer, strlen(buffer) + 1);
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function = nirio_driver_iface::NIRIO_FUNC::SET_STRING;
        in.params.attributeStr.attribute = attribute;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy::peek(uint32_t offset, uint32_t& value)
    {
        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;

        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function         = nirio_driver_iface::NIRIO_FUNC::IO;
        in.subfunction      = nirio_driver_iface::NIRIO_IO::PEEK32;
        in.params.io.offset = offset;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        value = out.params.io.value.value32;
        return status;
    }

    nirio_status niriok_proxy::peek(uint32_t offset, uint64_t& value)
    {
        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;

        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function         = nirio_driver_iface::NIRIO_FUNC::IO;
        in.subfunction      = nirio_driver_iface::NIRIO_IO::PEEK64;
        in.params.io.offset = offset;

        nirio_status status = sync_operation(&in, sizeof(in), &out, sizeof(out));
        value = out.params.io.value.value64;
        return status;
    }

    nirio_status niriok_proxy::poke(uint32_t offset, const uint32_t& value)
    {
        if (offset % 4 != 0) return NiRio_Status_MisalignedAccess;

        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function                 = nirio_driver_iface::NIRIO_FUNC::IO;
        in.subfunction              = nirio_driver_iface::NIRIO_IO::POKE32;
        in.params.io.offset         = offset;
        in.params.io.value.value32  = value;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy::poke(uint32_t offset, const uint64_t& value)
    {
        if (offset % 8 != 0) return NiRio_Status_MisalignedAccess;

        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function                 = nirio_driver_iface::NIRIO_FUNC::IO;
        in.subfunction              = nirio_driver_iface::NIRIO_IO::POKE64;
        in.params.io.offset         = offset;
        in.params.io.value.value64  = value;

        return sync_operation(&in, sizeof(in), &out, sizeof(out));
    }

    nirio_status niriok_proxy::map_fifo_memory(
        uint32_t fifo_instance,
        size_t size,
        nirio_driver_iface::rio_mmap_t& map)
    {
        return nirio_driver_iface::rio_mmap(_device_handle,
                GET_FIFO_MEMORY_TYPE(fifo_instance),
                size, true, map);
    }

    nirio_status niriok_proxy::unmap_fifo_memory(
        nirio_driver_iface::rio_mmap_t& map)
    {
        return nirio_driver_iface::rio_munmap(map);
    }
}}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif
