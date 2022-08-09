//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <iostream>

using namespace uhd::rfnoc;

dirtifier_t node_t::ALWAYS_DIRTY{};


node_t::node_t()
{
    _resolve_all_cb = _default_resolve_all_cb;
    register_property(&ALWAYS_DIRTY);
}

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

void node_t::set_properties(const uhd::device_addr_t& props, const size_t instance)
{
    for (const auto& key : props.keys()) {
        std::string local_key  = key;
        size_t local_instance  = instance;
        const size_t colon_pos = key.find(':');
        if (colon_pos != std::string::npos) {
            // Extract the property ID and instance
            local_key                 = key.substr(0, colon_pos);
            std::string instance_part = key.substr(colon_pos + 1);
            try {
                local_instance = std::stoi(instance_part);
            } catch (...) {
                // If no number, or an invalid number is specified after the
                // colon, throw a value_error.
                throw uhd::value_error("Property id `" + local_key
                                       + "' contains a malformed instance override!");
            }
        }

        property_base_t* prop_ref =
            _find_property({res_source_info::USER, local_instance}, local_key);
        if (!prop_ref) {
            RFNOC_LOG_WARNING("set_properties() cannot set property `"
                              << local_key << "': No such property.");
            continue;
        }
        auto prop_access = _request_property_access(prop_ref, property_base_t::RW);
        prop_ref->set_from_str(props.get(key));
    }

    // Now trigger a property resolution. If other properties depend on modified
    // properties, they will be updated.
    resolve_all();
}

void node_t::set_command_time(uhd::time_spec_t time, const size_t instance)
{
    if (_cmd_timespecs.size() <= instance) {
        _cmd_timespecs.resize(instance + 1, uhd::time_spec_t(0.0));
    }

    _cmd_timespecs[instance] = time;
}

uhd::time_spec_t node_t::get_command_time(const size_t instance) const
{
    if (instance >= _cmd_timespecs.size()) {
        return uhd::time_spec_t::ASAP;
    }

    return _cmd_timespecs.at(instance);
}

void node_t::clear_command_time(const size_t instance)
{
    set_command_time(uhd::time_spec_t(0.0), instance);
}

/*** Protected methods *******************************************************/
void node_t::register_property(property_base_t* prop, resolve_callback_t&& clean_callback)
{
    std::lock_guard<std::mutex> _l(_prop_mutex);
    const auto src_type = prop->get_src_info().type;

    // If the map is empty for this source type, create an empty vector
    if (_props.count(src_type) == 0) {
        _props[src_type] = {};
    }

    auto prop_already_registered = [prop](const property_base_t* existing_prop) {
        return (prop == existing_prop)
               || (prop->get_src_info() == existing_prop->get_src_info()
                      && prop->get_id() == existing_prop->get_id());
    };
    if (!filter_props(prop_already_registered).empty()) {
        throw uhd::runtime_error(std::string("Attempting to double-register property: ")
                                 + prop->get_id() + "[" + prop->get_src_info().to_string()
                                 + "]");
    }

    _props[src_type].push_back(prop);
    if (clean_callback) {
        _clean_cb_registry[prop] = std::move(clean_callback);
    }

    prop_accessor_t{}.set_access(prop, property_base_t::RW);
}

void node_t::add_property_resolver(
    prop_ptrs_t&& inputs, prop_ptrs_t&& outputs, resolver_fn_t&& resolver_fn)
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
    _prop_resolvers.push_back(std::make_tuple(std::forward<prop_ptrs_t>(inputs),
        std::forward<prop_ptrs_t>(outputs),
        std::forward<resolver_fn_t>(resolver_fn)));
}


void node_t::set_prop_forwarding_policy(
    forwarding_policy_t policy, const std::string& prop_id)
{
    _prop_fwd_policies[prop_id] = policy;
}

void node_t::set_prop_forwarding_map(const forwarding_map_t& map)
{
    _prop_fwd_map = map;
}

void node_t::register_action_handler(const std::string& id, action_handler_t&& handler)
{
    if (_action_handlers.count(id)) {
        _action_handlers.erase(id);
    }
    _action_handlers.emplace(id, std::move(handler));
}

void node_t::set_action_forwarding_policy(
    node_t::forwarding_policy_t policy, const std::string& action_key)
{
    _action_fwd_policies[action_key] = policy;
}

void node_t::set_action_forwarding_map(const forwarding_map_t& map)
{
    _action_fwd_map = map;
}

void node_t::post_action(const res_source_info& edge_info, action_info::sptr action)
{
    _post_action_cb(edge_info, action);
}

bool node_t::check_topology(const std::vector<size_t>& connected_inputs,
    const std::vector<size_t>& connected_outputs)
{
    for (size_t port : connected_inputs) {
        if (port >= get_num_input_ports()) {
            return false;
        }
    }
    for (size_t port : connected_outputs) {
        if (port >= get_num_output_ports()) {
            return false;
        }
    }

    return true;
}

/*** Private methods *********************************************************/
property_base_t* node_t::_find_property(
    res_source_info src_info, const std::string& id) const
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


property_base_t* node_t::inject_edge_property(
    property_base_t* blueprint, res_source_info new_src_info)
{
    // Check if a property already exists which matches the new property
    // requirements. If so, we can return early:
    auto new_prop = _find_property(new_src_info, blueprint->get_id());
    if (new_prop) {
        return new_prop;
    }

    // We need to create a new property and stash it away:
    new_prop = [&]() -> property_base_t* {
        auto prop = blueprint->clone(new_src_info);
        auto ptr  = prop.get();
        _dynamic_props.emplace(std::move(prop));
        return ptr;
    }();
    register_property(new_prop);

    // Collect some info on how to do the forwarding:
    const auto fwd_policy = [&](const std::string& id) {
        if (_prop_fwd_policies.count(id)) {
            return _prop_fwd_policies.at(id);
        }
        return _prop_fwd_policies.at("");
    }(new_prop->get_id());
    const size_t port_idx = new_prop->get_src_info().instance;
    const auto port_type  = new_prop->get_src_info().type;
    UHD_ASSERT_THROW(port_type == res_source_info::INPUT_EDGE
                     || port_type == res_source_info::OUTPUT_EDGE);

    // Now comes the hard part: Figure out which other properties need to be
    // created, and which resolvers need to be instantiated
    if (fwd_policy == forwarding_policy_t::ONE_TO_ONE) {
        // Figure out if there's an opposite port
        const auto opposite_port_type = res_source_info::invert_edge(port_type);
        if (_has_port({opposite_port_type, port_idx})) {
            // Make sure that the other side's property exists:
            // This is a safe recursion, because we've already created and
            // registered this property.
            auto opposite_prop =
                inject_edge_property(new_prop, {opposite_port_type, port_idx});
            // Now add a resolver that will always forward the value from this
            // property to the other one.
            add_property_resolver(
                {new_prop}, {opposite_prop}, [new_prop, opposite_prop]() {
                    prop_accessor_t{}.forward<false>(new_prop, opposite_prop);
                });
        }
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_FAN) {
        const auto opposite_port_type = res_source_info::invert_edge(port_type);
        const size_t num_ports        = opposite_port_type == res_source_info::INPUT_EDGE
                                     ? get_num_input_ports()
                                     : get_num_output_ports();
        for (size_t i = 0; i < num_ports; i++) {
            auto opposite_prop = inject_edge_property(new_prop, {opposite_port_type, i});
            // Now add a resolver that will always forward the value from this
            // property to the other one.
            add_property_resolver(
                {new_prop}, {opposite_prop}, [new_prop, opposite_prop]() {
                    prop_accessor_t{}.forward<false>(new_prop, opposite_prop);
                });
        }
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_ALL
        || fwd_policy == forwarding_policy_t::ONE_TO_ALL_IN) {
        // Loop through all other ports, make sure those properties exist
        for (size_t other_port_idx = 0; other_port_idx < get_num_input_ports();
             other_port_idx++) {
            if (port_type == res_source_info::INPUT_EDGE && other_port_idx == port_idx) {
                continue;
            }
            inject_edge_property(new_prop, {res_source_info::INPUT_EDGE, other_port_idx});
        }
        // Now add a dynamic resolver that will update all input properties.
        // In order to keep this code simple, we bypass the write list and
        // get access via the prop_accessor.
        add_property_resolver({new_prop}, {/* empty */}, [this, new_prop, port_idx]() {
            for (size_t other_port_idx = 0; other_port_idx < get_num_input_ports();
                 other_port_idx++) {
                if (other_port_idx == port_idx) {
                    continue;
                }
                auto prop = _find_property(
                    {res_source_info::INPUT_EDGE, other_port_idx}, new_prop->get_id());
                if (prop) {
                    prop_accessor_t{}.forward<false>(new_prop, prop);
                }
            }
        });
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_ALL
        || fwd_policy == forwarding_policy_t::ONE_TO_ALL_OUT) {
        // Loop through all other ports, make sure those properties exist
        for (size_t other_port_idx = 0; other_port_idx < get_num_output_ports();
             other_port_idx++) {
            if (port_type == res_source_info::OUTPUT_EDGE && other_port_idx == port_idx) {
                continue;
            }
            inject_edge_property(
                new_prop, {res_source_info::OUTPUT_EDGE, other_port_idx});
        }
        // Now add a dynamic resolver that will update all input properties.
        // In order to keep this code simple, we bypass the write list and
        // get access via the prop_accessor.
        add_property_resolver({new_prop}, {/* empty */}, [this, new_prop, port_idx]() {
            for (size_t other_port_idx = 0; other_port_idx < get_num_input_ports();
                 other_port_idx++) {
                if (other_port_idx == port_idx) {
                    continue;
                }
                auto prop = _find_property(
                    {res_source_info::OUTPUT_EDGE, other_port_idx}, new_prop->get_id());
                if (prop) {
                    prop_accessor_t{}.forward<false>(new_prop, prop);
                }
            }
        });
    }
    if (fwd_policy == forwarding_policy_t::USE_MAP) {
        const auto src_info   = new_prop->get_src_info();
        const auto& map_entry = _prop_fwd_map.find(src_info);
        if (map_entry != _prop_fwd_map.end()) {
            for (const auto& dst : map_entry->second) {
                if (!_has_port(dst)) {
                    throw uhd::rfnoc_error("Destination port " + dst.to_string()
                                           + " in prop map does not exist");
                }
                auto next_prop = inject_edge_property(new_prop, dst);
                // Now add a resolver that will always forward the value from this
                // property to the other one.
                add_property_resolver({new_prop}, {next_prop}, [new_prop, next_prop]() {
                    prop_accessor_t{}.forward<false>(new_prop, next_prop);
                });
            }
        } else {
            RFNOC_LOG_TRACE("Dropping incoming prop on " << src_info.to_string()
                                                         << " (no destinations in map)");
        }
    }

    return new_prop;
}


void node_t::init_props()
{
    std::lock_guard<std::mutex> _l(_prop_mutex);

    prop_accessor_t prop_accessor{};

    for (auto& resolver_tuple : _prop_resolvers) {
        // 1) Set all outputs to RWLOCKED
        auto& outputs = std::get<1>(resolver_tuple);
        for (auto& output : outputs) {
            prop_accessor.set_access(output, property_base_t::RWLOCKED);
        }

        // 2) Run the resolver
        try {
            std::get<2>(resolver_tuple)();
        } catch (const uhd::resolve_error& ex) {
            UHD_LOGGER_WARNING(get_unique_id())
                << "Failed to initialize node. Most likely cause: Inconsistent default "
                   "values. Resolver threw this error: "
                << ex.what();
            // throw uhd::runtime_error(std::string("Failed to initialize node ") +
            // get_unique_id());
        }

        // 3) Set outputs back to RO
        for (auto& output : outputs) {
            prop_accessor.set_access(output, property_base_t::RO);
        }
    }

    // 4) Mark properties as clean and read-only
    clean_props();
}


void node_t::resolve_props()
{
    prop_accessor_t prop_accessor{};
    const prop_ptrs_t initial_dirty_props =
        filter_props([](property_base_t* prop) { return prop->is_dirty(); });
    std::list<property_base_t*> all_dirty_props(
        initial_dirty_props.cbegin(), initial_dirty_props.cend());
    std::unordered_set<property_base_t*> processed_props{};
    std::unordered_set<property_base_t*> written_props{};
    RFNOC_LOG_TRACE("Locally resolving " << all_dirty_props.size()
                                         << " dirty properties plus dependencies.");

    // Loop through all dirty properties. The list can be amended during the
    // loop execution.
    for (auto it = all_dirty_props.begin(); it != all_dirty_props.end(); ++it) {
        auto current_input_prop = *it;
        if (processed_props.count(current_input_prop)) {
            continue;
        }
        // Find all resolvers that take this dirty property as an input:
        for (auto& resolver_tuple : _prop_resolvers) {
            auto& inputs  = std::get<0>(resolver_tuple);
            auto& outputs = std::get<1>(resolver_tuple);
            if (!uhd::has(inputs, current_input_prop)) {
                continue;
            }

            // Enable outputs
            std::vector<uhd::utils::scope_exit::uptr> access_holder;
            access_holder.reserve(outputs.size());
            for (auto& output : outputs) {
                access_holder.emplace_back(prop_accessor.get_scoped_prop_access(*output,
                    written_props.count(output) ? property_base_t::access_t::RWLOCKED
                                                : property_base_t::access_t::RW));
            }

            // Run resolver
            std::get<2>(resolver_tuple)();

            // Take note of outputs
            written_props.insert(outputs.cbegin(), outputs.cend());

            // Add all outputs that are dirty to the list, unless they have
            // already been processed
            for (auto& output_prop : outputs) {
                if (output_prop->is_dirty() && processed_props.count(output_prop) == 0) {
                    all_dirty_props.push_back(output_prop);
                }
            }

            // RW or RWLOCKED gets released here as access_holder goes out of scope.
        }
        processed_props.insert(current_input_prop);
    }
}

void node_t::resolve_all()
{
    _resolve_all_cb();
}


void node_t::clean_props()
{
    prop_accessor_t prop_accessor{};
    for (const auto& type_prop_pair : _props) {
        for (const auto& prop : type_prop_pair.second) {
            if (prop->is_valid() && prop->is_dirty() && _clean_cb_registry.count(prop)) {
                _clean_cb_registry.at(prop)();
            }
            prop_accessor.mark_clean(*prop);
            prop_accessor.set_access(prop, property_base_t::RO);
        }
    }
}


void node_t::forward_edge_property(
    property_base_t* incoming_prop, const size_t incoming_port)
{
    UHD_ASSERT_THROW(
        incoming_prop->get_src_info().type == res_source_info::INPUT_EDGE
        || incoming_prop->get_src_info().type == res_source_info::OUTPUT_EDGE);
    RFNOC_LOG_TRACE("Incoming edge property: `"
                    << incoming_prop->get_id()
                    << "`, source info: " << incoming_prop->get_src_info().to_string());

    // Don't forward properties that are not yet valid
    if (!incoming_prop->is_valid()) {
        RFNOC_LOG_TRACE("Skipped empty edge property: `"
                        << incoming_prop->get_id() << "`, source info: "
                        << incoming_prop->get_src_info().to_string());
        return;
    }

    // The source type of my local prop (it's the opposite of the source type
    // of incoming_prop)
    const auto prop_src_type =
        res_source_info::invert_edge(incoming_prop->get_src_info().type);
    // List of local properties that match incoming_prop. It can be an empty list,
    // or, if the node is misconfigured, have more than one entry. Or, if
    // all is as expected, it's a list with a single entry.
    auto local_prop_list = filter_props(
        [prop_src_type, incoming_prop, incoming_port](property_base_t* prop) -> bool {
            return prop->get_src_info().type == prop_src_type
                   && prop->get_src_info().instance == incoming_port
                   && prop->get_id() == incoming_prop->get_id();
        });

    // If there is no such property, we're forwarding a new property
    if (local_prop_list.empty()) {
        RFNOC_LOG_TRACE(
            "Received unknown incoming edge prop: " << incoming_prop->get_id());
        local_prop_list.push_back(
            inject_edge_property(incoming_prop, {prop_src_type, incoming_port}));
    }
    // There must be either zero results, or one
    UHD_ASSERT_THROW(local_prop_list.size() == 1);

    auto local_prop = *local_prop_list.begin();

    prop_accessor_t prop_accessor{};
    prop_accessor.forward<false>(incoming_prop, local_prop);
}

void node_t::receive_action(const res_source_info& src_info, action_info::sptr action)
{
    std::lock_guard<std::mutex> l(_action_mutex);
    // See if the user defined an action handler for us:
    if (_action_handlers.count(action->key)) {
        _action_handlers.at(action->key)(src_info, action);
        return;
    }

    // We won't forward actions if they were for us
    if (src_info.type == res_source_info::USER) {
        RFNOC_LOG_TRACE("Dropping USER action " << action->key << "#" << action->id);
        return;
    }

    // Otherwise, we need to figure out the correct default action handling:
    const auto fwd_policy = [&](const std::string& id) {
        if (_action_fwd_policies.count(id)) {
            return _action_fwd_policies.at(id);
        }
        return _action_fwd_policies.at("");
    }(action->key);

    // Now implement custom forwarding for all forwarding policies:
    if (fwd_policy == forwarding_policy_t::DROP) {
        RFNOC_LOG_TRACE("Dropping action " << action->key);
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_ONE) {
        RFNOC_LOG_TRACE("Forwarding action " << action->key << " to opposite port");
        const res_source_info dst_info{
            res_source_info::invert_edge(src_info.type), src_info.instance};
        if (_has_port(dst_info)) {
            post_action(dst_info, action);
        }
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_FAN) {
        RFNOC_LOG_TRACE("Forwarding action " << action->key << " to all opposite ports");
        const auto new_edge_type = res_source_info::invert_edge(src_info.type);
        const size_t num_ports   = new_edge_type == res_source_info::INPUT_EDGE
                                     ? get_num_input_ports()
                                     : get_num_output_ports();
        for (size_t i = 0; i < num_ports; i++) {
            post_action({new_edge_type, i}, action);
        }
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_ALL
        || fwd_policy == forwarding_policy_t::ONE_TO_ALL_IN) {
        RFNOC_LOG_TRACE("Forwarding action " << action->key << " to all input ports");
        for (size_t i = 0; i < get_num_input_ports(); i++) {
            if (src_info.type == res_source_info::INPUT_EDGE && i == src_info.instance) {
                continue;
            }
            post_action({res_source_info::INPUT_EDGE, i}, action);
        }
    }
    if (fwd_policy == forwarding_policy_t::ONE_TO_ALL
        || fwd_policy == forwarding_policy_t::ONE_TO_ALL_OUT) {
        RFNOC_LOG_TRACE("Forwarding action " << action->key << " to all output ports");
        for (size_t i = 0; i < get_num_output_ports(); i++) {
            if (src_info.type == res_source_info::OUTPUT_EDGE && i == src_info.instance) {
                continue;
            }
            post_action({res_source_info::OUTPUT_EDGE, i}, action);
        }
    }
    if (fwd_policy == forwarding_policy_t::USE_MAP) {
        const auto& map_entry = _action_fwd_map.find(src_info);
        if (map_entry != _action_fwd_map.end()) {
            for (const auto& dst : map_entry->second) {
                if (!_has_port(dst)) {
                    throw uhd::rfnoc_error("Destination port " + dst.to_string()
                                           + " in action map does not exist");
                }
                RFNOC_LOG_TRACE(
                    "Forwarding action " << action->key << " to " << dst.to_string());
                post_action(dst, action);
            }
        } else {
            RFNOC_LOG_TRACE("Dropping action " << action->key << " on "
                                               << src_info.to_string()
                                               << " (no destinations in map)");
        }
    }
}

void node_t::shutdown()
{
    RFNOC_LOG_DEBUG("shutdown() not implemented.");
}

bool node_t::_has_port(const res_source_info& port_info) const
{
    return (port_info.type == res_source_info::INPUT_EDGE
               && port_info.instance < get_num_input_ports())
           || (port_info.type == res_source_info::OUTPUT_EDGE
                  && port_info.instance < get_num_output_ports());
}
