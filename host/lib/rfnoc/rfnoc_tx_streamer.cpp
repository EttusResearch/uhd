//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer.hpp>
#include <atomic>

using namespace uhd;
using namespace uhd::rfnoc;

const std::string STREAMER_ID = "TxStreamer";
static std::atomic<uint64_t> streamer_inst_ctr;

rfnoc_tx_streamer::rfnoc_tx_streamer(const size_t num_chans,
    const uhd::stream_args_t stream_args)
    : tx_streamer_impl<chdr_tx_data_xport>(num_chans, stream_args)
    , _unique_id(STREAMER_ID + "#" + std::to_string(streamer_inst_ctr++))
    , _stream_args(stream_args)
{
    // No block to which to forward properties or actions
    set_prop_forwarding_policy(forwarding_policy_t::DROP);
    set_action_forwarding_policy(forwarding_policy_t::DROP);

    // Initialize properties
    _scaling_out.reserve(num_chans);
    _samp_rate_out.reserve(num_chans);
    _tick_rate_out.reserve(num_chans);
    _type_out.reserve(num_chans);

    for (size_t i = 0; i < num_chans; i++) {
        _register_props(i, stream_args.otw_format);
    }

    node_accessor_t node_accessor;
    node_accessor.init_props(this);
}

std::string rfnoc_tx_streamer::get_unique_id() const
{
    return _unique_id;
}

size_t rfnoc_tx_streamer::get_num_input_ports() const
{
    return 0;
}

size_t rfnoc_tx_streamer::get_num_output_ports() const
{
    return get_num_channels();
}

const uhd::stream_args_t& rfnoc_tx_streamer::get_stream_args() const
{
    return _stream_args;
}

bool rfnoc_tx_streamer::check_topology(
    const std::vector<size_t>& connected_inputs,
    const std::vector<size_t>& connected_outputs)
{
    // Check that all channels are connected
    if (connected_outputs.size() != get_num_output_ports()) {
        return false;
    }

    // Call base class to check that connections are valid
    return node_t::check_topology(connected_inputs, connected_outputs);
}

void rfnoc_tx_streamer::_register_props(const size_t chan,
    const std::string& otw_format)
{
    // Create actual properties and store them
    _scaling_out.push_back(property_t<double>(
        PROP_KEY_SCALING, {res_source_info::OUTPUT_EDGE, chan}));
    _samp_rate_out.push_back(property_t<double>(
        PROP_KEY_SAMP_RATE, {res_source_info::OUTPUT_EDGE, chan}));
    _tick_rate_out.push_back(property_t<double>(
        PROP_KEY_TICK_RATE, {res_source_info::OUTPUT_EDGE, chan}));
    _type_out.emplace_back(property_t<std::string>(
        PROP_KEY_TYPE, otw_format, {res_source_info::OUTPUT_EDGE, chan}));

    // Give us some shorthands for the rest of this function
    property_t<double>* scaling_out   = &_scaling_out.back();
    property_t<double>* samp_rate_out = &_samp_rate_out.back();
    property_t<double>* tick_rate_out = &_tick_rate_out.back();
    property_t<std::string>* type_out = &_type_out.back();

    // Register them
    register_property(scaling_out);
    register_property(samp_rate_out);
    register_property(tick_rate_out);
    register_property(type_out);

    // Add resolvers
    add_property_resolver({scaling_out}, {},
        [&scaling_out = *scaling_out, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `scaling_out'@" << chan);
            if (scaling_out.is_valid()) {
                this->set_scale_factor(chan, 32767.0 / scaling_out.get());
            }
        });

    add_property_resolver({samp_rate_out}, {},
        [&samp_rate_out = *samp_rate_out, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `samp_rate_out'@" << chan);
            if (samp_rate_out.is_valid()) {
                this->set_samp_rate(samp_rate_out.get());
            }
        });

    add_property_resolver({tick_rate_out}, {},
        [&tick_rate_out = *tick_rate_out, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `tick_rate_out'@" << chan);
            if (tick_rate_out.is_valid()) {
                this->set_tick_rate(tick_rate_out.get());
            }
        });
}
