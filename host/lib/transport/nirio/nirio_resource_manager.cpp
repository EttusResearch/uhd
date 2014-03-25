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


#include <uhd/transport/nirio/nirio_resource_manager.h>

#ifdef __clang__
    #pragma GCC diagnostic push ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace uhd { namespace niusrprio
{

nirio_resource_manager::nirio_resource_manager(
    niriok_proxy& proxy) : _kernel_proxy(proxy), _fifo_info_map(), _reg_info_map()
{
}

nirio_resource_manager::~nirio_resource_manager()
{
    finalize();
}

nirio_status nirio_resource_manager::initialize(
    const nirio_register_info_vtr& reg_info_vtr,
    const nirio_fifo_info_vtr& fifo_info_vtr)
{
    nirio_status status = 0;
    for (nirio_fifo_info_vtr::const_iterator it = fifo_info_vtr.begin(); it != fifo_info_vtr.end(); it++) {
        const nirio_fifo_info_t& fifo_info = *it;
        status = _add_fifo_resource(fifo_info);
        if (nirio_status_fatal(status)) return status;

        _fifo_info_map.insert(fifo_info_map_t::value_type(fifo_info.name, fifo_info));
    }
    for (nirio_register_info_vtr::const_iterator it = reg_info_vtr.begin(); it != reg_info_vtr.end(); it++) {
        const nirio_register_info_t& reg_info = *it;

        _reg_info_map.insert(register_info_map_t::value_type(reg_info.name, reg_info));
    }
    return _set_driver_config();
}

void nirio_resource_manager::finalize()
{
    _fifo_info_map.clear();
}

nirio_status nirio_resource_manager::get_register_offset(
    const char* register_name,
    uint32_t& offset)
{
    register_info_map_t::const_iterator it = _reg_info_map.find(fifo_info_map_t::key_type(register_name));
    if (it == _reg_info_map.end()) return NiRio_Status_InvalidParameter;

    offset = (*it).second.offset;

    return NiRio_Status_Success;
}


nirio_status nirio_resource_manager::_add_fifo_resource(
    const nirio_fifo_info_t& fifo_info)
{
    nirio_driver_iface::nirio_syncop_in_params_t in = {};
    nirio_driver_iface::nirio_syncop_out_params_t out = {};

    in.function    = nirio_driver_iface::NIRIO_FUNC::ADD_RESOURCE;
    in.subfunction = (fifo_info.direction == OUTPUT_FIFO) ?
            nirio_driver_iface::NIRIO_RESOURCE::OUTPUT_FIFO :
            nirio_driver_iface::NIRIO_RESOURCE::INPUT_FIFO;

    in.params.add.fifoWithDataType.channel        = fifo_info.channel;
    in.params.add.fifoWithDataType.baseAddress    = fifo_info.base_addr;
    in.params.add.fifoWithDataType.depthInSamples = fifo_info.depth;
    in.params.add.fifoWithDataType.scalarType     = fifo_info.scalar_type;
    in.params.add.fifoWithDataType.bitWidth       = fifo_info.width;
    in.params.add.fifoWithDataType.version        = fifo_info.version;

    return _kernel_proxy.sync_operation(&in, sizeof(in), &out, sizeof(out));
}

nirio_status nirio_resource_manager::_set_driver_config()
{
    nirio_driver_iface::nirio_syncop_in_params_t in = {};
    nirio_driver_iface::nirio_syncop_out_params_t out = {};
    in.function    = nirio_driver_iface::NIRIO_FUNC::SET_DRIVER_CONFIG;
    in.subfunction = 0;

    return _kernel_proxy.sync_operation(&in, sizeof(in), &out, sizeof(out));
}

nirio_fifo_info_t* nirio_resource_manager::_lookup_fifo_info(const char* fifo_name) {
    fifo_info_map_t::iterator it = _fifo_info_map.find(fifo_info_map_t::key_type(fifo_name));
    if (it == _fifo_info_map.end()) return NULL;

    return &((*it).second);
}

}}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif
