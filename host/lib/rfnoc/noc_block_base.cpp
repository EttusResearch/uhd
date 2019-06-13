//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>

using namespace uhd::rfnoc;

noc_block_base::make_args_t::~make_args_t() = default;

/******************************************************************************
 * Structors
 *****************************************************************************/
noc_block_base::noc_block_base(make_args_ptr make_args)
    : register_iface_holder(std::move(make_args->reg_iface))
    , _noc_id(make_args->noc_id)
    , _block_id(make_args->block_id)
    , _num_input_ports(make_args->num_input_ports)
    , _num_output_ports(make_args->num_output_ports)
    , _ctrlport_clock_iface(make_args->ctrlport_clk_iface)
    , _tb_clock_iface(make_args->tb_clk_iface)
    , _mb_controller(std::move(make_args->mb_control))
    , _block_args(make_args->args)
    , _tree(make_args->tree)
{
    RFNOC_LOG_TRACE(
        "Using timebase clock: `" << _tb_clock_iface->get_name() << "'. Current frequency: "
                                 << (_tb_clock_iface->get_freq() / 1e6) << " MHz");
    RFNOC_LOG_TRACE("Using ctrlport clock: `"
                    << _ctrlport_clock_iface->get_name() << "'. Current frequency: "
                    << (_ctrlport_clock_iface->get_freq() / 1e6) << " MHz");
    // First, create one tick_rate property for every port
    _tick_rate_props.reserve(get_num_input_ports() + get_num_output_ports());
    for (size_t input_port = 0; input_port < get_num_input_ports(); input_port++) {
        _tick_rate_props.push_back(property_t<double>(PROP_KEY_TICK_RATE,
            _tb_clock_iface->get_freq(),
            {res_source_info::INPUT_EDGE, input_port}));
    }
    for (size_t output_port = 0; output_port < get_num_output_ports(); output_port++) {
        _tick_rate_props.push_back(property_t<double>(PROP_KEY_TICK_RATE,
            _tb_clock_iface->get_freq(),
            {res_source_info::OUTPUT_EDGE, output_port}));
    }
    // Register all the tick_rate properties and create a default resolver
    prop_ptrs_t prop_refs;
    prop_refs.reserve(_tick_rate_props.size());
    for (auto& prop : _tick_rate_props) {
        prop_refs.insert(&prop);
        register_property(&prop);
    }
    for (auto& prop : _tick_rate_props) {
        auto prop_refs_copy = prop_refs;
        add_property_resolver(
            {&prop}, std::move(prop_refs_copy), [this, source_prop = &prop]() {
                // _set_tick_rate() will update _tick_rate, but only if that's
                // a valid operation for this block
                this->_set_tick_rate(source_prop->get());
                // Now, _tick_rate is valid and we will pass its value to all
                // tick_rate properties
                for (property_t<double>& tick_rate_prop : _tick_rate_props) {
                    tick_rate_prop = get_tick_rate();
                }
            });
    }
}

noc_block_base::~noc_block_base()
{
    for (const auto& node : _tree->list("")) {
        _tree->remove(node);
    }
}

void noc_block_base::set_num_input_ports(const size_t num_ports)
{
    if (num_ports > get_num_input_ports()) {
        throw uhd::value_error(
            "New number of input ports must not exceed current number!");
    }

    _num_input_ports = num_ports;
}

void noc_block_base::set_num_output_ports(const size_t num_ports)
{
    if (num_ports > get_num_output_ports()) {
        throw uhd::value_error(
            "New number of output ports must not exceed current number!");
    }

    _num_output_ports = num_ports;
}


double noc_block_base::get_tick_rate() const
{
    return _tb_clock_iface->get_freq();
}

void noc_block_base::set_tick_rate(const double tick_rate)
{
    if (tick_rate == get_tick_rate()) {
        return;
    }
    // Update this node
    RFNOC_LOG_TRACE("Setting tb clock freq to " << tick_rate/1e6 << " MHz");
    _tb_clock_iface->set_freq(tick_rate);
    // Now trigger property propagation
    if (!_tick_rate_props.empty()) {
        auto src_info = _tick_rate_props.at(0).get_src_info();
        set_property<double>(PROP_KEY_TICK_RATE, tick_rate, src_info);
    }
}

void noc_block_base::_set_tick_rate(const double tick_rate)
{
    if (tick_rate == get_tick_rate()) {
        return;
    }
    if (tick_rate <= 0) {
        RFNOC_LOG_WARNING("Attempting to set tick rate to 0. Skipping.")
        return;
    }
    if (_tb_clock_iface->get_name() == CLOCK_KEY_GRAPH) {
        RFNOC_LOG_TRACE("Updating tick rate to " << (tick_rate / 1e6) << " MHz");
        _tb_clock_iface->set_freq(tick_rate);
    } else {
        RFNOC_LOG_WARNING("Cannot change tick rate to "
                          << (tick_rate / 1e6)
                          << " MHz, this clock is not configurable by the graph!");
    }
}

void noc_block_base::shutdown()
{
    RFNOC_LOG_TRACE("Calling deinit()");
    deinit();
    RFNOC_LOG_DEBUG("Invalidating register interface");
    update_reg_iface();
}

std::shared_ptr<mb_controller> noc_block_base::get_mb_controller()
{
    return _mb_controller;
}

void noc_block_base::deinit()
{
    RFNOC_LOG_DEBUG("deinit() called, but not implemented.");
}
