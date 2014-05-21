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


#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_RESOURCE_MANAGER_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_RESOURCE_MANAGER_H

#include <uhd/transport/nirio/nirio_fifo.h>
#include <uhd/transport/nirio/niriok_proxy.h>
#include <vector>
#include <map>
#include <string>
#include <stdint.h>

namespace uhd { namespace niusrprio
{
enum register_direction_t {
    CONTROL,
    INDICATOR
};

struct nirio_register_info_t {
    nirio_register_info_t(
        uint32_t                 arg_offset,
        const char*             arg_name,
        register_direction_t    arg_direction) :
            offset(arg_offset),
            name(arg_name),
            direction(arg_direction)
    {}

    uint32_t                 offset;
    std::string                name;
    register_direction_t    direction;
};

typedef std::vector<nirio_register_info_t> nirio_register_info_vtr;

struct nirio_fifo_info_t {
    nirio_fifo_info_t(
        uint32_t             arg_channel,
        const char*         arg_name,
        fifo_direction_t    arg_direction,
        uint32_t             arg_base_addr,
        uint32_t             arg_depth,
        nirio_scalar_t         arg_scalar_type,
        uint32_t            arg_width,
        uint32_t             arg_version) :
            channel(arg_channel),
            name(arg_name),
            direction(arg_direction),
            base_addr(arg_base_addr),
            depth(arg_depth),
            scalar_type(arg_scalar_type),
            width(arg_width),
            version(arg_version)
    {}

    uint32_t             channel;
    std::string            name;
    fifo_direction_t    direction;
    uint32_t             base_addr;
    uint32_t             depth;
    nirio_scalar_t         scalar_type;
    uint32_t            width;
    uint32_t             version;
};

typedef std::vector<nirio_fifo_info_t> nirio_fifo_info_vtr;


class nirio_resource_manager
{
public:
    nirio_resource_manager(niriok_proxy& proxy);
    virtual ~nirio_resource_manager();

    nirio_status initialize(const nirio_register_info_vtr& reg_info_vtr, const nirio_fifo_info_vtr& fifo_info_vtr);
    void finalize();

    nirio_status get_register_offset(const char* register_name, uint32_t& offset);

    template<typename data_t>
    nirio_status create_tx_fifo(const char* fifo_name, boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        nirio_fifo_info_t* fifo_info_ptr = _lookup_fifo_info(fifo_name);
        if (fifo_info_ptr) {
            fifo.reset(new nirio_fifo<data_t>(_kernel_proxy, OUTPUT_FIFO, fifo_info_ptr->name, fifo_info_ptr->channel));
        } else {
            return NiRio_Status_ResourceNotFound;
        }

        if (fifo->get_channel() != fifo_info_ptr->channel) return NiRio_Status_InvalidParameter;
        if (fifo->get_scalar_type() != fifo_info_ptr->scalar_type) return NiRio_Status_InvalidParameter;

        return NiRio_Status_Success;
    }

    template<typename data_t>
    nirio_status create_rx_fifo(const char* fifo_name, boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        nirio_fifo_info_t* fifo_info_ptr = _lookup_fifo_info(fifo_name);
        if (fifo_info_ptr) {
            fifo.reset(new nirio_fifo<data_t>(_kernel_proxy, INPUT_FIFO, fifo_info_ptr->name, fifo_info_ptr->channel));
        } else {
            return NiRio_Status_ResourceNotFound;
        }

        if (fifo->get_channel() != fifo_info_ptr->channel) return NiRio_Status_InvalidParameter;
        if (fifo->get_scalar_type() != fifo_info_ptr->scalar_type) return NiRio_Status_InvalidParameter;

        return NiRio_Status_Success;
    }

private:
    nirio_resource_manager (const nirio_resource_manager&);
    nirio_resource_manager& operator = (const nirio_resource_manager&);

    typedef std::map<const std::string, nirio_fifo_info_t> fifo_info_map_t;
    typedef std::map<const std::string, nirio_register_info_t> register_info_map_t;

    nirio_status _add_fifo_resource(const nirio_fifo_info_t& fifo_info);
    nirio_status _set_driver_config();
    nirio_fifo_info_t* _lookup_fifo_info(const char* fifo_name);

    niriok_proxy&           _kernel_proxy;
    fifo_info_map_t            _fifo_info_map;
    register_info_map_t        _reg_info_map;
};

}}
#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_RESOURCE_MANAGER_H */
