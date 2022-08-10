//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/format.hpp>
#include <boost/units/detail/utility.hpp>

namespace {

template <typename prop_data_t>
uhd::rfnoc::property_t<prop_data_t>* _assert_prop(
    uhd::rfnoc::property_base_t* prop_base_ptr,
    const std::string& node_id,
    const std::string& prop_id)
{
    // First check if the pointer is valid at all:
    if (prop_base_ptr == nullptr) {
        throw uhd::lookup_error(
            str(boost::format("[%s] Unknown property: `%s'") % node_id % prop_id));
    }

    // Next, check if we can cast the pointer to the desired type:
    auto prop_ptr =
        dynamic_cast<uhd::rfnoc::property_t<prop_data_t>*>(prop_base_ptr);
    if (!prop_ptr) {
        throw uhd::type_error(str(
            boost::format(
                "[%s] Found property `%s', but could not cast to requested type `%s'!")
            % node_id % prop_id % boost::units::detail::demangle(typeid(prop_data_t).name()) ));
    }

    // All is good, we now return the raw pointer that has been validated.
    return prop_ptr;
}

} // namespace

namespace uhd { namespace rfnoc {

template <typename prop_data_t>
void node_t::set_property(
    const std::string& id, const prop_data_t& val, const size_t instance)
{
    res_source_info src_info{res_source_info::USER, instance};
    set_property<prop_data_t>(id, val, src_info);
}

template <typename prop_data_t>
const prop_data_t& node_t::get_property(const std::string& id, const size_t instance)
{
    res_source_info src_info{res_source_info::USER, instance};
    return get_property<prop_data_t>(id, src_info);
}

template <typename prop_data_t>
void node_t::set_property(
    const std::string& id, const prop_data_t& val, const res_source_info& src_info)
{
    if(_graph_mutex_cb) {
        // Node connected to graph. Must lock graph first.
        std::lock_guard<std::recursive_mutex> l(_graph_mutex_cb());
        _set_property(id, val, src_info);
    }
    else {
        // Node unconnected to graph
        _set_property(id, val, src_info);
    }
}

template <typename prop_data_t>
const prop_data_t& node_t::get_property(
    const std::string& id, const res_source_info& src_info)
{
    RFNOC_LOG_TRACE("Getting property " << id << "@" << src_info.to_string());
    // First, trigger a property resolution to make sure this property is
    // updated (if necessary) before reading it out
    resolve_all();
    auto prop_ptr = _assert_prop<prop_data_t>(
        _find_property(src_info, id), get_unique_id(), id);

    auto prop_access = _request_property_access(prop_ptr, property_base_t::RO);
    return prop_ptr->get();
}

template <typename prop_data_t>
void node_t::_set_property(
    const std::string& id, const prop_data_t& val, const res_source_info& src_info)
{
    RFNOC_LOG_TRACE("Setting property " << id << "@" << src_info.to_string());

    auto prop_ptr =
        _assert_prop<prop_data_t>(_find_property(src_info, id), get_unique_id(), id);
    {
        auto prop_access = _request_property_access(prop_ptr, property_base_t::RW);
        prop_ptr->set(val);
    }

    // Now trigger a property resolution. If other properties depend on this one,
    // they will be updated.
    resolve_all();
}

}} /* namespace uhd::rfnoc */

