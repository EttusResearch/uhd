//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/rfnoc_rx_streamer.hpp>
#include <atomic>
#include <thread>

using namespace std::chrono_literals;
;
using namespace uhd;
using namespace uhd::rfnoc;

const std::string STREAMER_ID = "RxStreamer";
static std::atomic<uint64_t> streamer_inst_ctr;

rfnoc_rx_streamer::rfnoc_rx_streamer(const size_t num_chans,
    const uhd::stream_args_t stream_args)
    : rx_streamer_impl<chdr_rx_data_xport>(num_chans, stream_args)
    , _unique_id(STREAMER_ID + "#" + std::to_string(streamer_inst_ctr++))
    , _stream_args(stream_args)
{
    // No block to which to forward properties or actions
    set_prop_forwarding_policy(forwarding_policy_t::DROP);
    set_action_forwarding_policy(forwarding_policy_t::DROP);
    //
    register_action_handler(ACTION_KEY_RX_EVENT,
        [this](const res_source_info& src, action_info::sptr action) {
            rx_event_action_info::sptr rx_event_action =
                std::dynamic_pointer_cast<rx_event_action_info>(action);
            if (!rx_event_action) {
                RFNOC_LOG_WARNING("Received invalid RX event action!");
                return;
            }
            _handle_rx_event_action(src, rx_event_action);
        });
    register_action_handler(ACTION_KEY_RX_RESTART_REQ,
        [this](const res_source_info& src, action_info::sptr action) {
            _handle_restart_request(src, action);
        });
    register_action_handler(ACTION_KEY_STREAM_CMD,
        [this](const res_source_info& src, action_info::sptr action) {
            stream_cmd_action_info::sptr stream_cmd_action =
                std::dynamic_pointer_cast<stream_cmd_action_info>(action);
            if (!stream_cmd_action) {
                RFNOC_LOG_WARNING("Received invalid stream command action!");
                return;
            }
            _handle_stream_cmd_action(src, stream_cmd_action);
        });

    // Initialize properties
    _scaling_in.reserve(num_chans);
    _samp_rate_in.reserve(num_chans);
    _tick_rate_in.reserve(num_chans);
    _type_in.reserve(num_chans);

    for (size_t i = 0; i < num_chans; i++) {
        _register_props(i, stream_args.otw_format);
    }
    node_accessor_t node_accessor{};
    node_accessor.init_props(this);
}

std::string rfnoc_rx_streamer::get_unique_id() const
{
    return _unique_id;
}

size_t rfnoc_rx_streamer::get_num_input_ports() const
{
    return get_num_channels();
}

size_t rfnoc_rx_streamer::get_num_output_ports() const
{
    return 0;
}

void rfnoc_rx_streamer::issue_stream_cmd(const stream_cmd_t& stream_cmd)
{
    if (get_num_channels() > 1 and stream_cmd.stream_now
        and stream_cmd.stream_mode != stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS) {
        throw uhd::runtime_error(
            "Invalid recv stream command - stream now on multiple channels in a "
            "single streamer will fail to time align.");
    }

    auto cmd = stream_cmd_action_info::make(stream_cmd.stream_mode);
    cmd->stream_cmd = stream_cmd;

    for (size_t i = 0; i < get_num_channels(); i++) {
        const res_source_info info(res_source_info::INPUT_EDGE, i);
        post_action(info, cmd);
    }
}

const uhd::stream_args_t& rfnoc_rx_streamer::get_stream_args() const
{
    return _stream_args;
}

bool rfnoc_rx_streamer::check_topology(
    const std::vector<size_t>& connected_inputs,
    const std::vector<size_t>& connected_outputs)
{
    // Check that all channels are connected
    if (connected_inputs.size() != get_num_input_ports()) {
        return false;
    }

    // Call base class to check that connections are valid
    return node_t::check_topology(connected_inputs, connected_outputs);
}

void rfnoc_rx_streamer::_register_props(const size_t chan,
    const std::string& otw_format)
{
    // Create actual properties and store them
    _scaling_in.push_back(property_t<double>(
        PROP_KEY_SCALING, {res_source_info::INPUT_EDGE, chan}));
    _samp_rate_in.push_back(
        property_t<double>(PROP_KEY_SAMP_RATE, {res_source_info::INPUT_EDGE, chan}));
    _tick_rate_in.push_back(property_t<double>(
        PROP_KEY_TICK_RATE, {res_source_info::INPUT_EDGE, chan}));
    _type_in.emplace_back(property_t<std::string>(
        PROP_KEY_TYPE, otw_format, {res_source_info::INPUT_EDGE, chan}));

    // Give us some shorthands for the rest of this function
    property_t<double>* scaling_in   = &_scaling_in.back();
    property_t<double>* samp_rate_in = &_samp_rate_in.back();
    property_t<double>* tick_rate_in = &_tick_rate_in.back();
    property_t<std::string>* type_in = &_type_in.back();

    // Register them
    register_property(scaling_in);
    register_property(samp_rate_in);
    register_property(tick_rate_in);
    register_property(type_in);

    // Add resolvers
    add_property_resolver({scaling_in}, {},
        [&scaling_in = *scaling_in, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `scaling_in'@" << chan);
            if (scaling_in.is_valid()) {
                this->set_scale_factor(chan, scaling_in.get() / 32767.0);
            }
        });

    add_property_resolver({samp_rate_in}, {},
        [&samp_rate_in = *samp_rate_in, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `samp_rate_in'@" << chan);
            if (samp_rate_in.is_valid()) {
                this->set_samp_rate(samp_rate_in.get());
            }
        });

    add_property_resolver({tick_rate_in}, {},
        [&tick_rate_in = *tick_rate_in, chan, this]() {
            RFNOC_LOG_TRACE("Calling resolver for `tick_rate_in'@" << chan);
            if (tick_rate_in.is_valid()) {
                this->set_tick_rate(tick_rate_in.get());
            }
        });
}

void rfnoc_rx_streamer::_handle_rx_event_action(
    const res_source_info& src, rx_event_action_info::sptr rx_event_action)
{
    UHD_ASSERT_THROW(src.type == res_source_info::INPUT_EDGE);
    if (rx_event_action->error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
        RFNOC_LOG_DEBUG("Received overrun message on port " << src.instance);
        if (_overrun_handling_mode.exchange(true)) {
            RFNOC_LOG_TRACE("Ignoring duplicate overrun message.");
            return;
        }
        RFNOC_LOG_TRACE(
            "Switching to overrun-handling mode: Stopping all upstream producers...");
        auto stop_action =
            stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        // Reminder: Delivery of all of these actions is deferred until this
        // action handler is complete.
        for (size_t i = 0; i < get_num_input_ports(); ++i) {
            post_action({res_source_info::INPUT_EDGE, i}, stop_action);
        }
        if (!rx_event_action->args.cast<bool>("cont_mode", false)) {
            // FIXME wait until it's safe to restart radios
            // If we don't need to restart, that's all we need to do
            _overrun_handling_mode = false;
        }
    }
}

void rfnoc_rx_streamer::_handle_restart_request(
    const res_source_info& src, action_info::sptr)
{
    // FIXME: Now we need to wait until it's safe to restart the radios.
    // A flush would achieve this, albeit at the cost of possibly losing
    // samples.
    // The earliest we can restart is when the FIFOs in upstream producers
    // are empty.
    RFNOC_LOG_TRACE("Waiting for FIFOs to clear");

    std::this_thread::sleep_for(100ms);

    // Once it's safe to restart the radios, we ask a radio to send us a
    // stream command with its current time.
    RFNOC_LOG_TRACE("Requesting restart from overrun-reporting node...");
    post_action({res_source_info::INPUT_EDGE, src.instance},
        action_info::make(ACTION_KEY_RX_RESTART_REQ));
}

void rfnoc_rx_streamer::_handle_stream_cmd_action(
    const res_source_info& src, stream_cmd_action_info::sptr stream_cmd_action)
{
    RFNOC_LOG_TRACE("Received stream command on " << src.to_string());
    UHD_ASSERT_THROW(src.type == res_source_info::INPUT_EDGE);
    auto start_action =
        stream_cmd_action_info::make(stream_cmd_action->stream_cmd.stream_mode);
    start_action->stream_cmd = stream_cmd_action->stream_cmd;
    for (size_t i = 0; i < get_num_input_ports(); ++i) {
        post_action({res_source_info::INPUT_EDGE, i}, start_action);
    }
    if (_overrun_handling_mode.exchange(false)) {
        RFNOC_LOG_TRACE("Leaving overrun handling mode.");
    }
}
