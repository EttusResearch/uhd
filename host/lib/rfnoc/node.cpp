//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/node.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;


std::string node_t::get_unique_id() const
{
    // TODO return something better
    return str(boost::format("%08X") % this);
}

std::vector<std::string> node_t::get_property_ids() const
{
    std::lock_guard<std::mutex> _l(_prop_mutex);
    if (_props.count(res_source_info::USER) == 0) {
        return {};
    }

    auto& user_props = _props.at(res_source_info::USER);
    // TODO use a range here, we're not savages
    std::vector<std::string> return_value(user_props.size());
    for (size_t i = 0; i < user_props.size(); ++i) {
        return_value[i] = user_props[i]->get_id();
    }

    return return_value;
}

/*** Protected methods *******************************************************/
void node_t::register_property(property_base_t* prop)
{
    std::lock_guard<std::mutex> _l(_prop_mutex);

    const auto src_type          = prop->get_src_info().type;
    auto prop_already_registered = [prop](const property_base_t* existing_prop) {
        return (prop->get_src_info() == existing_prop->get_src_info()
                   && prop->get_id() == existing_prop->get_id())
               || (prop == existing_prop);
    };

    // If the map is empty for this source type, create an empty vector
    if (_props.count(src_type) == 0) {
        _props[src_type] = {};
    }

    // Now go and make sure no one has registered this property before
    auto& props = _props[src_type];
    for (const auto& existing_prop : props) {
        if (prop_already_registered(existing_prop)) {
            throw uhd::runtime_error("Attempting to double-register prop");
        }
    }

    _props[src_type].push_back(prop);
}

void node_t::add_property_resolver(std::set<property_base_t*>&& inputs,
    std::set<property_base_t*>&& outputs,
    resolver_fn_t&& resolver_fn)
{
    std::lock_guard<std::mutex> _l(_prop_mutex);

    // Sanity check: All inputs and outputs must be registered properties
    auto prop_is_registered = [this](property_base_t* prop) -> bool {
        return bool(this->_find_property(prop->get_src_info(), prop->get_id()));
    };
    for (const auto& prop : inputs) {
        if (!prop_is_registered(prop)) {
            throw uhd::runtime_error(
                std::string("Cannot add property resolver, input property ")
                + prop->get_id() + " is not registered!");
        }
    }
    for (const auto& prop : outputs) {
        if (!prop_is_registered(prop)) {
            throw uhd::runtime_error(
                std::string("Cannot add property resolver, output property ")
                + prop->get_id() + " is not registered!");
        }
    }

    // All good, we can store it
    _prop_resolvers.push_back(std::make_tuple(
            std::forward<std::set<property_base_t*>>(inputs),
            std::forward<std::set<property_base_t*>>(outputs),
            std::forward<resolver_fn_t>(resolver_fn)));
}

/*** Private methods *********************************************************/
property_base_t* node_t::_find_property(res_source_info src_info, const std::string& id) const
{
    for (const auto& type_prop_pair : _props) {
        if (type_prop_pair.first != src_info.type) {
            continue;
        }
        for (const auto& prop : type_prop_pair.second) {
            if (prop->get_id() == id && prop->get_src_info() == src_info) {
                return prop;
            }
        }
    }

    return nullptr;
}

uhd::utils::scope_exit::uptr node_t::_request_property_access(
    property_base_t* prop, property_base_t::access_t access) const
{
    return prop_accessor_t{}.get_scoped_prop_access(*prop, access);
}

