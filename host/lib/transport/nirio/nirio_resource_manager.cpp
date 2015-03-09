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

// "push" and "pop" introduced in GCC 4.6; works with all clang
#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace uhd { namespace niusrprio
{

nirio_resource_manager::nirio_resource_manager():_fifo_info_map(), _reg_info_map()
{
}

void nirio_resource_manager::set_proxy(niriok_proxy::sptr proxy)
{
   _kernel_proxy = proxy;
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
    return _kernel_proxy->add_fifo_resource(fifo_info);
}

nirio_status nirio_resource_manager::_set_driver_config()
{
    return _kernel_proxy->set_device_config();
}

nirio_fifo_info_t* nirio_resource_manager::_lookup_fifo_info(const char* fifo_name) {
    fifo_info_map_t::iterator it = _fifo_info_map.find(fifo_info_map_t::key_type(fifo_name));
    if (it == _fifo_info_map.end()) return NULL;

    return &((*it).second);
}

}}

#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic pop
#endif
