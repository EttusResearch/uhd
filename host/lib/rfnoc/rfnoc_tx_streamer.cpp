//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer.hpp>
#include <atomic>
#include <numeric>

using namespace uhd;
using namespace uhd::rfnoc;

const std::string STREAMER_ID = "TxStreamer";
static std::atomic<uint64_t> streamer_inst_ctr;
static constexpr size_t ASYNC_MSG_QUEUE_SIZE = 1000;

rfnoc_tx_streamer::rfnoc_tx_streamer(const size_t num_chans,
    const uhd::stream_args_t stream_args,
    disconnect_fn_t disconnect_cb)
    : tx_streamer_impl<chdr_tx_data_xport>(num_chans, stream_args)
    , _unique_id(STREAMER_ID + "#" + std::to_string(streamer_inst_ctr++))
    , _stream_args(stream_args)
    , _disconnect_cb(disconnect_cb)
{
    _async_msg_queue = std::make_shared<tx_async_msg_queue>(ASYNC_MSG_QUEUE_SIZE);

    // No block to which to forward properties or actions
    set_prop_forwarding_policy(forwarding_policy_t::DROP);
    set_action_forwarding_policy(forwarding_policy_t::DROP);

    register_action_handler(ACTION_KEY_TX_EVENT,
        [this](const res_source_info& src, action_info::sptr action) {
            tx_event_action_info::sptr tx_event_action =
                std::dynamic_pointer_cast<tx_event_action_info>(action);
            if (!tx_event_action) {
                RFNOC_LOG_WARNING("Received invalid TX event action!");
                return;
            }
            _handle_tx_event_action(src, tx_event_action);
        });

    register_action_handler(ACTION_KEY_TUNE_REQUEST,
        [this](const res_source_info& src, action_info::sptr action) {
            tune_request_action_info::sptr tune_request_action =
                std::dynamic_pointer_cast<tune_request_action_info>(action);
            if (!tune_request_action) {
                RFNOC_LOG_WARNING("Received invalid tune request action!");
                return;
            }
            RFNOC_LOG_DEBUG("Received tune request on " << src.to_string());
        });

    // Initialize properties
    _scaling_out.reserve(num_chans);
    _samp_rate_out.reserve(num_chans);
    _tick_rate_out.reserve(num_chans);
    _type_out.reserve(num_chans);
    _mtu_out.reserve(num_chans);
    _atomic_item_size_out.reserve(num_chans);

    for (size_t i = 0; i < num_chans; i++) {
        _register_props(i, stream_args.otw_format);
    }

    for (size_t i = 0; i < num_chans; i++) {
        prop_ptrs_t mtu_resolver_out;
        mtu_resolver_out.reserve(_mtu_out.size());
        for (auto& mtu_prop : _mtu_out) {
            mtu_resolver_out.push_back(&mtu_prop);
        }

        add_property_resolver({&_mtu_out[i]},
            std::move(mtu_resolver_out),
            [&mtu_out = _mtu_out[i], i, this]() {
                const auto UHD_UNUSED(ii) = i;
                RFNOC_LOG_TRACE("Calling resolver for `mtu_out'@" << i);
                if (mtu_out.is_valid()) {
                    const size_t mtu =
                        std::min(mtu_out.get(), tx_streamer_impl::get_mtu());
                    // Set the same MTU value for all chans
                    for (auto& prop : this->_mtu_out) {
                        prop.set(mtu);
                    }
                    if (mtu < tx_streamer_impl::get_mtu()) {
                        tx_streamer_impl::set_mtu(mtu);
                    }
                }
            });
    }

    node_accessor_t node_accessor;
    node_accessor.init_props(this);
}

rfnoc_tx_streamer::~rfnoc_tx_streamer()
{
    if (_disconnect_cb) {
        _disconnect_cb(_unique_id);
    }
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

void rfnoc_tx_streamer::post_output_action(
    const std::shared_ptr<uhd::rfnoc::action_info>& action, const size_t port)
{
    if (port > get_num_channels()) {
        throw uhd::runtime_error("Invalid channel. Please provide a valid channel");
    }
    const uhd::rfnoc::res_source_info info(res_source_info::OUTPUT_EDGE, port);
    post_action(info, action);
}

const uhd::stream_args_t& rfnoc_tx_streamer::get_stream_args() const
{
    return _stream_args;
}

bool rfnoc_tx_streamer::check_topology(const std::vector<size_t>& connected_inputs,
    const std::vector<size_t>& connected_outputs)
{
    // Check that all channels are connected
    if (connected_outputs.size() != get_num_output_ports()) {
        return false;
    }

    // Call base class to check that connections are valid
    return node_t::check_topology(connected_inputs, connected_outputs);
}

void rfnoc_tx_streamer::connect_channel(
    const size_t channel, chdr_tx_data_xport::uptr xport)
{
    UHD_ASSERT_THROW(channel < _mtu_out.size());

    // Stash away the MTU before we lose access to xports
    const size_t mtu = xport->get_mtu();

    xport->set_enqueue_async_msg_fn(
        [this, channel](
            async_metadata_t::event_code_t event_code, bool has_tsf, uint64_t tsf) {
            async_metadata_t md;
            md.channel       = channel;
            md.event_code    = event_code;
            md.has_time_spec = has_tsf;

            if (has_tsf) {
                md.time_spec = time_spec_t::from_ticks(tsf, get_tick_rate());
            }

            this->_async_msg_queue->enqueue(md);
        });

    tx_streamer_impl<chdr_tx_data_xport>::connect_channel(channel, std::move(xport));

    const size_t bpi          = convert::get_bytes_per_item(_type_out[channel].get());
    const size_t chdr_w_bytes = _stream_args.args.cast<size_t>("__chdr_width", 64) / 8;
    const size_t chdr_w_items = chdr_w_bytes / bpi;
    const size_t spp          = tx_streamer_impl<chdr_tx_data_xport>::get_max_num_samps();
    const size_t misalignment = spp % chdr_w_items;
    if (!_stream_args.args.has_key("spp")) {
        // By default, find an spp value that is an integer multiple of the CHDR
        // width. This has proven useful under some corner conditions, but still
        // requires a root-cause analysis.
        if (misalignment != 0) {
            RFNOC_LOG_DEBUG("Reducing spp from "
                            << spp << " to " << spp - misalignment
                            << " to align with CHDR width of " << chdr_w_bytes
                            << " bytes (= " << chdr_w_items << " samples).");
            tx_streamer_impl<chdr_tx_data_xport>::set_max_num_samps(spp - misalignment);
        }
    }

    // Update MTU property based on xport limits. We need to do this after
    // connect_channel(), because that's where the chdr_tx_data_xport object
    // learns its header size.
    set_property<size_t>(PROP_KEY_MTU, mtu, {res_source_info::OUTPUT_EDGE, channel});
}

bool rfnoc_tx_streamer::recv_async_msg(
    uhd::async_metadata_t& async_metadata, double timeout)
{
    const auto timeout_ms = static_cast<uint64_t>(timeout * 1000);
    return _async_msg_queue->recv_async_msg(async_metadata, timeout_ms);
}

void rfnoc_tx_streamer::_register_props(const size_t chan, const std::string& otw_format)
{
    // Create actual properties and store them
    _scaling_out.push_back(
        property_t<double>(PROP_KEY_SCALING, {res_source_info::OUTPUT_EDGE, chan}));
    _samp_rate_out.push_back(
        property_t<double>(PROP_KEY_SAMP_RATE, {res_source_info::OUTPUT_EDGE, chan}));
    _tick_rate_out.push_back(
        property_t<double>(PROP_KEY_TICK_RATE, {res_source_info::OUTPUT_EDGE, chan}));
    _type_out.emplace_back(property_t<std::string>(
        PROP_KEY_TYPE, otw_format, {res_source_info::OUTPUT_EDGE, chan}));
    _mtu_out.push_back(property_t<size_t>(
        PROP_KEY_MTU, get_mtu(), {res_source_info::OUTPUT_EDGE, chan}));
    _atomic_item_size_out.push_back(property_t<size_t>(
        PROP_KEY_ATOMIC_ITEM_SIZE, 1, {res_source_info::OUTPUT_EDGE, chan}));

    // Give us some shorthands for the rest of this function
    property_t<double>* scaling_out          = &_scaling_out.back();
    property_t<double>* samp_rate_out        = &_samp_rate_out.back();
    property_t<double>* tick_rate_out        = &_tick_rate_out.back();
    property_t<std::string>* type_out        = &_type_out.back();
    property_t<size_t>* mtu_out              = &_mtu_out.back();
    property_t<size_t>* atomic_item_size_out = &_atomic_item_size_out.back();

    // Register them
    register_property(scaling_out);
    register_property(samp_rate_out);
    register_property(tick_rate_out);
    register_property(type_out);
    register_property(mtu_out);
    register_property(atomic_item_size_out);

    // Add resolvers
    add_property_resolver({scaling_out}, {}, [&scaling_out = *scaling_out, chan, this]() {
        RFNOC_LOG_TRACE("Calling resolver for `scaling_out'@" << chan);
        // Other data types than sc16 will require other values
        const double converter_scaling = 32767.0;
        const double graph_scaling     = scaling_out.is_valid() ? scaling_out.get() : 1.0;
        this->set_scale_factor(chan, converter_scaling * graph_scaling);
    });

    add_property_resolver(
        {samp_rate_out}, {}, [&samp_rate_out = *samp_rate_out, chan, this]() {
            const auto UHD_UNUSED(log_chan) = chan;
            RFNOC_LOG_TRACE("Calling resolver for `samp_rate_out'@" << chan);
            if (samp_rate_out.is_valid()) {
                this->set_samp_rate(samp_rate_out.get());
            }
        });

    add_property_resolver(
        {tick_rate_out}, {}, [&tick_rate_out = *tick_rate_out, chan, this]() {
            const auto UHD_UNUSED(log_chan) = chan;
            RFNOC_LOG_TRACE("Calling resolver for `tick_rate_out'@" << chan);
            if (tick_rate_out.is_valid()) {
                this->set_tick_rate(tick_rate_out.get());
            }
        });
    add_property_resolver({atomic_item_size_out, mtu_out, type_out},
        {},
        [&ais = *atomic_item_size_out, &type = *type_out, chan, this]() {
            const auto UHD_UNUSED(log_chan) = chan;
            RFNOC_LOG_TRACE("Calling resolver for `atomic_item_size'@" << chan);
            if (ais.is_valid()) {
                const size_t bpi          = convert::get_bytes_per_item(type.get());
                const size_t spp          = this->tx_streamer_impl::get_max_num_samps();
                const size_t spp_multiple = std::lcm<size_t>(ais.get(), bpi) / bpi;
                if (spp < spp_multiple) {
                    RFNOC_LOG_ERROR("Cannot resolve spp! Must be a multiple of "
                                    << spp_multiple << " but max value is " << spp);
                    throw uhd::value_error(
                        "Samples per packet is incompatible with atomic item size!");
                }
                const auto misalignment = spp % spp_multiple;
                if (misalignment > 0) {
                    RFNOC_LOG_DEBUG("Reducing spp from " << spp << " to "
                                                         << spp - misalignment
                                                         << " to align with atomic "
                                                            "item size");
                    this->tx_streamer_impl::set_max_num_samps(spp - misalignment);
                }
            }
        });
}

void rfnoc_tx_streamer::_handle_tx_event_action(
    const res_source_info& src, tx_event_action_info::sptr tx_event_action)
{
    UHD_ASSERT_THROW(src.type == res_source_info::OUTPUT_EDGE);

    uhd::async_metadata_t md;
    md.event_code    = tx_event_action->event_code;
    md.channel       = src.instance;
    md.has_time_spec = tx_event_action->has_tsf;

    if (md.has_time_spec) {
        md.time_spec = time_spec_t::from_ticks(tx_event_action->tsf, get_tick_rate());
    }

    RFNOC_LOG_TRACE("Pushing metadata onto tx async msg queue, channel " << md.channel);
    _async_msg_queue->enqueue(md);
}
