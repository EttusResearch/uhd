//
// Copyright 2019-2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/filter_node.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/string.hpp>
#include <uhdlib/rfnoc/rfnoc_device.hpp>
#include <uhdlib/rfnoc/rfnoc_rx_streamer.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer_replay_buffered.hpp>
#include <uhdlib/usrp/gpio_defs.hpp>
#include <uhdlib/usrp/multi_usrp_utils.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <unordered_set>
#include <boost/format.hpp>
#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

//! Fan out (mux) an API call that is for all channels or all motherboards
#define MUX_API_CALL(max_index, api_call, mux_var, mux_cond, ...)  \
    if (mux_var == mux_cond) {                                     \
        for (size_t __index = 0; __index < max_index; ++__index) { \
            api_call(__VA_ARGS__, __index);                        \
        }                                                          \
        return;                                                    \
    }

//! Fan out (mux) an RX-specific API call that is for all channels
#define MUX_RX_API_CALL(api_call, ...) \
    MUX_API_CALL(get_rx_num_channels(), api_call, chan, ALL_CHANS, __VA_ARGS__)
//! Fan out (mux) an TX-specific API call that is for all channels
#define MUX_TX_API_CALL(api_call, ...) \
    MUX_API_CALL(get_tx_num_channels(), api_call, chan, ALL_CHANS, __VA_ARGS__)
//! Fan out (mux) a motherboard-specific API call that is for all boards
#define MUX_MB_API_CALL(api_call, ...) \
    MUX_API_CALL(get_num_mboards(), api_call, mboard, ALL_MBOARDS, __VA_ARGS__)


namespace {
constexpr char DEFAULT_CPU_FORMAT[] = "fc32";
constexpr char DEFAULT_OTW_FORMAT[] = "sc16";
constexpr double RX_SIGN            = +1.0;
constexpr double TX_SIGN            = -1.0;
constexpr char LOG_ID[]             = "MULTI_USRP";

//! A faux container for a UHD device
//
// Note that multi_usrp_rfnoc no longer gives access to the underlying device
// class. Legacy code might use multi_usrp->get_device()->get_tree() or
// similar functionalities; these can be faked with this redirector class.
//
// The only exception is recv_async_msg(), which depends on the streamer. It
// will print a warning once, and will attempt to access a Tx streamer if it
// has access to a Tx streamer. If there is only ever one Tx streamer, this will
// work as expected. For multiple streamers, only the last streamer's async
// messages will make it through.
class redirector_device : public uhd::device
{
public:
    redirector_device(multi_usrp* musrp_ptr) : _musrp(musrp_ptr) {
        _tree = musrp_ptr->get_tree();
    }

    rx_streamer::sptr get_rx_stream(const stream_args_t& args) override
    {
        return _musrp->get_rx_stream(args);
    }

    tx_streamer::sptr get_tx_stream(const stream_args_t& args) override
    {
        auto streamer     = _musrp->get_tx_stream(args);
        _last_tx_streamer = streamer;
        return streamer;
    }

    bool recv_async_msg(async_metadata_t& md, double timeout) override
    {
        std::call_once(_async_warning_flag, []() {
            UHD_LOG_WARNING(LOG_ID,
                "Calling multi_usrp::recv_async_msg() is deprecated and can lead to "
                "unexpected behaviour. Prefer calling tx_stream::recv_async_msg().");
        });
        auto streamer = _last_tx_streamer.lock();
        if (streamer) {
            return streamer->recv_async_msg(md, timeout);
        }
        return false;
    }

    device_filter_t get_device_type() const
    {
        return USRP;
    }

    void set_tx_stream(tx_streamer::sptr streamer)
    {
        _last_tx_streamer = streamer;
    }

private:
    std::once_flag _async_warning_flag;
    std::weak_ptr<tx_streamer> _last_tx_streamer;
    multi_usrp* _musrp;
};

/*! Make sure the stream args are valid and can be used by get_tx_stream()
 * and get_rx_stream().
 *
 */
stream_args_t sanitize_stream_args(const stream_args_t args_)
{
    stream_args_t args = args_;
    if (args.cpu_format.empty()) {
        UHD_LOG_DEBUG("MULTI_USRP",
            "get_xx_stream(): cpu_format not specified, defaulting to "
                << DEFAULT_CPU_FORMAT);
        args.cpu_format = DEFAULT_CPU_FORMAT;
    }
    if (args.otw_format.empty()) {
        UHD_LOG_DEBUG("MULTI_USRP",
            "get_xx_stream(): otw_format not specified, defaulting to "
                << DEFAULT_OTW_FORMAT);
        args.otw_format = DEFAULT_OTW_FORMAT;
    }
    if (args.channels.empty()) {
        UHD_LOG_DEBUG(
            "MULTI_USRP", "get_xx_stream(): channels not specified, defaulting to [0]");
        args.channels = {0};
    }
    return args;
}

std::string bytes_to_str(std::vector<uint8_t> str_b)
{
    return std::string(str_b.cbegin(), str_b.cend());
}

} // namespace

class multi_usrp_rfnoc : public multi_usrp
{
    using replay_config_t = rfnoc_tx_streamer_replay_buffered::replay_config_t;

public:
    struct rx_chan_t
    {
        radio_control::sptr radio;
        ddc_block_control::sptr ddc; // can be nullptr
        size_t block_chan;
        std::vector<graph_edge_t> edge_list;
    };

    struct tx_chan_t
    {
        radio_control::sptr radio;
        duc_block_control::sptr duc; // can be nullptr
        size_t block_chan;
        std::vector<graph_edge_t> edge_list;
        replay_config_t replay;
    };

    /**************************************************************************
     * Structors
     *************************************************************************/
    multi_usrp_rfnoc(rfnoc_graph::sptr graph, const device_addr_t& addr)
        : _args(addr)
        , _graph(graph)
        , _tree(_graph->get_tree())
        , _device(std::make_shared<redirector_device>(this))
    {
        // Discover all of the radios on our devices and create a mapping between
        // radio chains and channel numbers.  Do this one motherboard at a time
        // because each device can have a different subdev spec.
        size_t musrp_rx_channel = 0;
        size_t musrp_tx_channel = 0;
        for (size_t mboard = 0; mboard < get_num_mboards(); mboard++)
        {
            auto radio_blk_ids = _graph->find_blocks(std::to_string(mboard) + "/Radio");

            // Generate the RX and TX subdev specs
            uhd::usrp::subdev_spec_t rx_radio_subdev;
            uhd::usrp::subdev_spec_t tx_radio_subdev;
            for (auto radio_id : radio_blk_ids) {
                auto radio_blk = _graph->get_block<uhd::rfnoc::radio_control>(radio_id);

                // Store radio blocks per mboard for quick retrieval
                _radios[mboard].push_back(radio_blk);

                for (size_t block_chan = 0; block_chan < radio_blk->get_num_output_ports();
                    ++block_chan) {
                    rx_radio_subdev.push_back(uhd::usrp::subdev_spec_pair_t(
                        radio_blk->get_slot_name(),
                        radio_blk->get_dboard_fe_from_chan(block_chan, uhd::RX_DIRECTION)));
                }

                for (size_t block_chan = 0; block_chan < radio_blk->get_num_input_ports();
                    ++block_chan) {
                    tx_radio_subdev.push_back(uhd::usrp::subdev_spec_pair_t(
                        radio_blk->get_slot_name(),
                        radio_blk->get_dboard_fe_from_chan(block_chan, uhd::TX_DIRECTION)));
                }
            }

            // Generate the TX and RX chans
            // Note that we don't want to connect blocks now; we will wait until we create and
            // connect a streamer. This gives us a little more time to figure out the desired
            // values of our properties (such as master clock)
            auto rx_chans =
                _generate_mboard_rx_chans(rx_radio_subdev, mboard);
            for (auto rx_chan : rx_chans) {
                _rx_chans.emplace(musrp_rx_channel, rx_chan);
                ++musrp_rx_channel;
            }
            auto tx_chans =
                _generate_mboard_tx_chans(tx_radio_subdev, mboard);
            for (auto tx_chan : tx_chans) {
                _tx_chans.emplace(musrp_tx_channel, tx_chan);
                ++musrp_tx_channel;
            }
        }

        if (_rx_chans.empty() and _tx_chans.empty()) {
            // If we don't find any valid radio chains, we don't have a multi_usrp object
            throw uhd::runtime_error(
                "[multi_usrp] No valid radio channels found in connected devices.");
        }

        // Manually propagate radio block sample rates to DDC/DUC blocks in order to allow
        // DDC/DUC blocks to have valid internal state before graph is (later) connected
        for (size_t rx_chan = 0; rx_chan < get_rx_num_channels(); ++rx_chan) {
            auto& rx_chain = _get_rx_chan(rx_chan);
            if (rx_chain.ddc) {
                rx_chain.ddc->set_input_rate(
                    rx_chain.radio->get_rate(), rx_chain.block_chan);
            }
        }
        for (size_t tx_chan = 0; tx_chan < get_tx_num_channels(); ++tx_chan) {
            auto& tx_chain = _get_tx_chan(tx_chan);
            if (tx_chain.duc) {
                tx_chain.duc->set_output_rate(
                    tx_chain.radio->get_rate(), tx_chain.block_chan);
            }
        }

        _graph->commit();
    }

    ~multi_usrp_rfnoc() override
    {
        // nop
    }

    device::sptr get_device(void) override
    {
        return _device;
    }

    uhd::property_tree::sptr get_tree() const override
    {
        return _tree;
    }

    rx_streamer::sptr get_rx_stream(const stream_args_t& args_) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        stream_args_t args = sanitize_stream_args(args_);
        double rate        = 1.0;

        // Note that we don't release the graph, which means that property
        // propagation is possible. This is necessary so we don't disrupt
        // existing streamers. We use the _graph_mutex to try and avoid any
        // property propagation where possible.

        // Connect the chains
        auto edges = _connect_rx_chains(args.channels);
        std::weak_ptr<rfnoc_graph> graph_ref(_graph);

        // Create the streamer
        // The disconnect callback must disconnect the entire chain because the radio
        // relies on the connections to determine what is enabled.
        auto rx_streamer = std::make_shared<rfnoc_rx_streamer>(
            args.channels.size(), args, [=](const std::string& id) {
                if (auto graph = graph_ref.lock()) {
                    graph->disconnect(id);
                    for (auto edge : edges) {
                        graph->disconnect(edge.src_blockid,
                            edge.src_port,
                            edge.dst_blockid,
                            edge.dst_port);
                    }
                }
            });

        // Connect the streamer
        for (size_t strm_port = 0; strm_port < args.channels.size(); ++strm_port) {
            auto rx_channel = args.channels.at(strm_port);
            auto rx_chain   = _get_rx_chan(rx_channel);
            if (rx_chain.edge_list.empty()) {
                throw uhd::runtime_error("Graph edge list is empty for rx channel "
                                         + std::to_string(rx_channel));
            }
            UHD_LOG_TRACE("MULTI_USRP",
                "Connecting " << rx_chain.edge_list.back().src_blockid << ":"
                              << rx_chain.edge_list.back().src_port
                              << " -> RxStreamer:" << strm_port);
            _graph->connect(rx_chain.edge_list.back().src_blockid,
                rx_chain.edge_list.back().src_port,
                rx_streamer,
                strm_port);
            const double chan_rate =
                _rx_rates.count(rx_channel) ? _rx_rates.at(rx_channel) : 1.0;
            if (chan_rate > 1.0 && rate != chan_rate) {
                if (rate > 1.0) {
                    UHD_LOG_DEBUG("MULTI_USRP",
                        "Inconsistent RX rates when creating streamer! "
                        "Harmonizing to "
                            << chan_rate);
                }
                rate = chan_rate;
            }
        }

        // Now everything is connected, commit() again so we can have stream
        // commands go through the graph
        _graph->commit();

        // Before we return the streamer, we may need to reapply the rate. This
        // is necessary whenever the blocks were configured before the streamer
        // was created, because we don't know what state the graph is in after
        // commit() was called in that case..
        if (rate > 1.0) {
            UHD_LOG_TRACE("MULTI_USRP",
                "Now reapplying RX rate " << (rate / 1e6)
                                          << " MHz to all streamer channels");
            for (auto rx_channel : args.channels) {
                auto rx_chain = _get_rx_chan(rx_channel);
                if (rx_chain.ddc) {
                    rx_chain.ddc->set_output_rate(rate, rx_chain.block_chan);
                } else {
                    rx_chain.radio->set_rate(rate);
                }
            }
        }

        return rx_streamer;
    }

    tx_streamer::sptr get_tx_stream(const stream_args_t& args_) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        stream_args_t args = sanitize_stream_args(args_);
        double rate        = 1.0;
        bool replay_buffered = (args.args.has_key("streamer") and
                            args.args["streamer"] == "replay_buffered");

        // Note that we don't release the graph, which means that property
        // propagation is possible. This is necessary so we don't disrupt
        // existing streamers. We use the _graph_mutex to try and avoid any
        // property propagation where possible.

        // Connect the chains
        std::map< size_t, std::vector<graph_edge_t> > edge_lists;
        std::vector<replay_config_t> replay_configs;
        for (auto channel : args.channels) {
            if (replay_buffered) {
                edge_lists[channel] = _connect_tx_chain_with_replay(channel);
                replay_configs.push_back(_get_tx_chan(channel).replay);
            } else {
                edge_lists[channel] = _connect_tx_chain(channel);
            }
        }

        // Create a streamer
        // The disconnect callback must disconnect the entire chain because the radio
        // relies on the connections to determine what is enabled.
        tx_streamer::sptr tx_streamer;
        std::weak_ptr<rfnoc_graph> graph_ref(_graph);
        auto disconnect = [=](const std::string& id) {
                if (auto graph = graph_ref.lock()) {
                    graph->disconnect(id);
                    for (auto edge_list : edge_lists) {
                        for (auto edge : edge_list.second) {
                            if (block_id_t(edge.src_blockid).match(NODE_ID_SEP) or
                                block_id_t(edge.dst_blockid).match(NODE_ID_SEP)) {
                                    continue;
                            }
                            graph->disconnect(edge.src_blockid,
                                edge.src_port,
                                edge.dst_blockid,
                                edge.dst_port);
                        }
                    }
                }
            };
        if (replay_buffered) {
            tx_streamer = std::make_shared<rfnoc_tx_streamer_replay_buffered>(
                    args.channels.size(), args, disconnect, replay_configs);
        } else {
            tx_streamer = std::make_shared<rfnoc_tx_streamer>(
                args.channels.size(), args, disconnect);
        }

        // Connect the streamer
        for (size_t strm_port = 0; strm_port < args.channels.size(); ++strm_port) {
            auto tx_channel = args.channels.at(strm_port);
            auto edge_list  = edge_lists[tx_channel];
            if (edge_list.empty()) {
                throw uhd::runtime_error("Graph edge list is empty for tx channel "
                                         + std::to_string(tx_channel));
            }
            UHD_LOG_TRACE("MULTI_USRP",
                "Connecting TxStreamer:" << strm_port << " -> "
                                         << edge_list.back().dst_blockid << ":"
                                         << edge_list.back().dst_port);
            _graph->connect(tx_streamer,
                strm_port,
                edge_list.back().dst_blockid,
                edge_list.back().dst_port);
            const double chan_rate =
                _tx_rates.count(tx_channel) ? _tx_rates.at(tx_channel) : 1.0;
            if (chan_rate > 1.0 && rate != chan_rate) {
                UHD_LOG_DEBUG("MULTI_USRP",
                    "Inconsistent TX rates when creating streamer! Harmonizing "
                    "to "
                        << chan_rate);
                rate = chan_rate;
            }
        }
        // Now everything is connected, commit() again so we can have stream
        // commands go through the graph
        _graph->commit();

        // Before we return the streamer, we may need to reapply the rate. This
        // is necessary whenever the blocks were configured before the streamer
        // was created, because we don't know what state the graph is in after
        // commit() was called in that case, or we could have configured blocks
        // to run at different rates (see the warning above).
        if (rate > 1.0) {
            UHD_LOG_TRACE("MULTI_USRP",
                "Now reapplying TX rate " << (rate / 1e6)
                                          << " MHz to all streamer channels");
            for (auto tx_channel : args.channels) {
                auto tx_chain = _get_tx_chan(tx_channel);
                if (tx_chain.duc) {
                    tx_chain.duc->set_input_rate(rate, tx_chain.block_chan);
                } else {
                    tx_chain.radio->set_rate(rate);
                }
            }
        }

        // For legacy purposes: This enables recv_async_msg(), which is considered
        // deprecated, but as long as it's there, we need this to approximate
        // previous behaviour.
        _device->set_tx_stream(tx_streamer);

        return tx_streamer;
    }


    dict<std::string, std::string> get_usrp_rx_info(size_t chan) override
    {
        auto& rx_chain      = _get_rx_chan(chan);
        const size_t mb_idx = rx_chain.radio->get_block_id().get_device_no();
        auto mbc            = _get_mbc(mb_idx);
        auto mb_eeprom      = mbc->get_eeprom();

        dict<std::string, std::string> usrp_info;
        usrp_info["mboard_id"]      = mbc->get_mboard_name();
        usrp_info["mboard_name"]    = mb_eeprom.get("name", "n/a");
        usrp_info["mboard_serial"]  = mb_eeprom.get("serial", "n/a");
        usrp_info["rx_subdev_name"] = get_rx_subdev_name(chan);
        usrp_info["rx_subdev_spec"] = get_rx_subdev_spec(mb_idx).to_string();
        usrp_info["rx_antenna"]     = get_rx_antenna(chan);

        const auto db_eeprom = rx_chain.radio->get_db_eeprom();
        usrp_info["rx_serial"] =
            db_eeprom.count("rx_serial")
                ? bytes_to_str(db_eeprom.at("rx_serial"))
                : db_eeprom.count("serial") ? bytes_to_str(db_eeprom.at("serial")) : "";
        usrp_info["rx_id"] =
            db_eeprom.count("rx_id")
                ? bytes_to_str(db_eeprom.at("rx_id"))
                : db_eeprom.count("pid") ? bytes_to_str(db_eeprom.at("pid")) : "";

        const auto rx_power_ref_keys = rx_chain.radio->get_rx_power_ref_keys(rx_chain.block_chan);
        if (!rx_power_ref_keys.empty() && rx_power_ref_keys.size() == 2) {
            usrp_info["rx_ref_power_key"]    = rx_power_ref_keys.at(0);
            usrp_info["rx_ref_power_serial"] = rx_power_ref_keys.at(1);
        }

        return usrp_info;
    }

    dict<std::string, std::string> get_usrp_tx_info(size_t chan) override
    {
        auto& tx_chain      = _get_tx_chan(chan);
        const size_t mb_idx = tx_chain.radio->get_block_id().get_device_no();
        auto mbc            = _get_mbc(mb_idx);
        auto mb_eeprom      = mbc->get_eeprom();

        dict<std::string, std::string> usrp_info;
        usrp_info["mboard_id"]      = mbc->get_mboard_name();
        usrp_info["mboard_name"]    = mb_eeprom.get("name", "n/a");
        usrp_info["mboard_serial"]  = mb_eeprom.get("serial", "n/a");
        usrp_info["tx_subdev_name"] = get_tx_subdev_name(chan);
        usrp_info["tx_subdev_spec"] = get_tx_subdev_spec(mb_idx).to_string();
        usrp_info["tx_antenna"]     = get_tx_antenna(chan);

        const auto db_eeprom = tx_chain.radio->get_db_eeprom();
        usrp_info["tx_serial"] =
            db_eeprom.count("tx_serial")
                ? bytes_to_str(db_eeprom.at("tx_serial"))
                : db_eeprom.count("serial") ? bytes_to_str(db_eeprom.at("serial")) : "";
        usrp_info["tx_id"] =
            db_eeprom.count("tx_id")
                ? bytes_to_str(db_eeprom.at("tx_id"))
                : db_eeprom.count("pid") ? bytes_to_str(db_eeprom.at("pid")) : "";

        const auto tx_power_ref_keys = tx_chain.radio->get_tx_power_ref_keys(tx_chain.block_chan);
        if (!tx_power_ref_keys.empty() && tx_power_ref_keys.size() == 2) {
            usrp_info["tx_ref_power_key"]    = tx_power_ref_keys.at(0);
            usrp_info["tx_ref_power_serial"] = tx_power_ref_keys.at(1);
        }

        return usrp_info;
    }

    /*! Tune the appropriate radio chain to the requested frequency.
     *  The general algorithm is the same for RX and TX, so we can pass in lambdas to do
     * the setting/getting for us.
     */
    tune_result_t tune_xx_subdev_and_dsp(const double xx_sign,
        freq_range_t tune_range,
        freq_range_t rf_freq_range,
        freq_range_t dsp_freq_range,
        std::function<void(double)> set_rf_freq,
        std::function<double()> get_rf_freq,
        std::function<void(double)> set_dsp_freq,
        std::function<double()> get_dsp_freq,
        const tune_request_t& tune_request)
    {
        double clipped_requested_freq = tune_range.clip(tune_request.target_freq);
        UHD_LOGGER_TRACE("MULTI_USRP")
            << boost::format("Frequency Range %.3fMHz->%.3fMHz")
                   % (tune_range.start() / 1e6) % (tune_range.stop() / 1e6);
        UHD_LOGGER_TRACE("MULTI_USRP")
            << "Clipped RX frequency requested: "
                   + std::to_string(clipped_requested_freq / 1e6) + "MHz";

        //------------------------------------------------------------------
        //-- set the RF frequency depending upon the policy
        //------------------------------------------------------------------
        double target_rf_freq = 0.0;
        switch (tune_request.rf_freq_policy) {
            case tune_request_t::POLICY_AUTO:
                target_rf_freq = clipped_requested_freq;
                break;

            case tune_request_t::POLICY_MANUAL:
                target_rf_freq = rf_freq_range.clip(tune_request.rf_freq);
                break;

            case tune_request_t::POLICY_NONE:
                break; // does not set
        }
        UHD_LOGGER_TRACE("MULTI_USRP")
            << "Target RF Freq: " + std::to_string(target_rf_freq / 1e6) + "MHz";

        //------------------------------------------------------------------
        //-- Tune the RF frontend
        //------------------------------------------------------------------
        if (tune_request.rf_freq_policy != tune_request_t::POLICY_NONE) {
            set_rf_freq(target_rf_freq);
        }
        const double actual_rf_freq = get_rf_freq();

        //------------------------------------------------------------------
        //-- Set the DSP frequency depending upon the DSP frequency policy.
        //------------------------------------------------------------------
        double target_dsp_freq = 0.0;
        switch (tune_request.dsp_freq_policy) {
            case tune_request_t::POLICY_AUTO:
                /* If we are using the AUTO tuning policy, then we prevent the
                 * CORDIC from spinning us outside of the range of the baseband
                 * filter, regardless of what the user requested. This could happen
                 * if the user requested a center frequency so far outside of the
                 * tunable range of the FE that the CORDIC would spin outside the
                 * filtered baseband. */
                target_dsp_freq = actual_rf_freq - clipped_requested_freq;

                // invert the sign on the dsp freq for transmit (spinning up vs down)
                target_dsp_freq *= xx_sign;

                break;

            case tune_request_t::POLICY_MANUAL:
                /* If the user has specified a manual tune policy, we will allow
                 * tuning outside of the baseband filter, but will still clip the
                 * target DSP frequency to within the bounds of the CORDIC to
                 * prevent undefined behavior (likely an overflow). */
                target_dsp_freq = dsp_freq_range.clip(tune_request.dsp_freq);
                break;

            case tune_request_t::POLICY_NONE:
                break; // does not set
        }
        UHD_LOGGER_TRACE("MULTI_USRP")
            << "Target DSP Freq: " + std::to_string(target_dsp_freq / 1e6) + "MHz";

        //------------------------------------------------------------------
        //-- Tune the DSP
        //------------------------------------------------------------------
        if (tune_request.dsp_freq_policy != tune_request_t::POLICY_NONE) {
            set_dsp_freq(target_dsp_freq);
        }
        const double actual_dsp_freq = get_dsp_freq();

        //------------------------------------------------------------------
        //-- Load and return the tune result
        //------------------------------------------------------------------
        tune_result_t tune_result;
        tune_result.clipped_rf_freq = clipped_requested_freq;
        tune_result.target_rf_freq  = target_rf_freq;
        tune_result.actual_rf_freq  = actual_rf_freq;
        tune_result.target_dsp_freq = target_dsp_freq;
        tune_result.actual_dsp_freq = actual_dsp_freq;
        return tune_result;
    }

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
    void set_master_clock_rate(double rate, size_t mboard) override
    {
        for (auto& chain : _rx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard
                || mboard == ALL_MBOARDS) {
                radio->set_rate(rate);
            }
        }
        for (auto& chain : _tx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard
                || mboard == ALL_MBOARDS) {
                radio->set_rate(rate);
            }
        }
    }

    double get_master_clock_rate(size_t mboard) override
    {
        // We pick the first radio we can find on this mboard, and hope that all
        // radios have the same range.
        for (auto& chain : _rx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard) {
                return radio->get_tick_rate();
            }
        }
        for (auto& chain : _tx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard) {
                return radio->get_tick_rate();
            }
        }
        throw uhd::key_error("Invalid mboard index!");
    }

    meta_range_t get_master_clock_rate_range(const size_t mboard) override
    {
        // We pick the first radio we can find on this mboard, and hope that all
        // radios have the same range.
        for (auto& chain : _rx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard) {
                return radio->get_rate_range();
            }
        }
        for (auto& chain : _tx_chans) {
            auto radio = chain.second.radio;
            if (radio->get_block_id().get_device_no() == mboard) {
                return radio->get_rate_range();
            }
        }
        throw uhd::key_error("Invalid mboard index!");
    }

    std::string get_pp_string(void) override
    {
        std::string buff = str(boost::format("%s USRP:\n"
                                             "  Device: %s\n")
                               % ((get_num_mboards() > 1) ? "Multi" : "Single")
                               % (_tree->access<std::string>("/name").get()));
        for (size_t m = 0; m < get_num_mboards(); m++) {
            buff += str(
                boost::format("  Mboard %d: %s\n") % m % _get_mbc(m)->get_mboard_name());
        }


        //----------- rx side of life ----------------------------------
        for (size_t rx_chan = 0; rx_chan < get_rx_num_channels(); rx_chan++) {
            buff += str(boost::format("  RX Channel: %u\n"
                                      "    RX DSP: %s\n"
                                      "    RX Dboard: %s\n"
                                      "    RX Subdev: %s\n")
                        % rx_chan
                        % (_rx_chans.at(rx_chan).ddc ? std::to_string(rx_chan) : "n/a")
                        % _rx_chans.at(rx_chan).radio->get_slot_name()
                        % get_rx_subdev_name(rx_chan));
        }

        //----------- tx side of life ----------------------------------
        for (size_t tx_chan = 0; tx_chan < get_tx_num_channels(); tx_chan++) {
            buff += str(boost::format("  TX Channel: %u\n"
                                      "    TX DSP: %s\n"
                                      "    TX Dboard: %s\n"
                                      "    TX Subdev: %s\n")
                        % tx_chan
                        % (_tx_chans.at(tx_chan).duc ? std::to_string(tx_chan) : "n/a")
                        % _tx_chans.at(tx_chan).radio->get_slot_name()
                        % get_tx_subdev_name(tx_chan));
        }

        return buff;
    }

    std::string get_mboard_name(size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_mboard_name();
    }

    time_spec_t get_time_now(size_t mboard = 0) override
    {
        return _radios[mboard][0]->get_time_now();
    }

    time_spec_t get_time_last_pps(size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_timekeeper(0)->get_time_last_pps();
    }

    void set_time_now(const time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_time_now, time_spec);
        _get_mbc(mboard)->get_timekeeper(0)->set_time_now(time_spec);
    }

    void set_time_next_pps(
        const time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_time_next_pps, time_spec);
        _get_mbc(mboard)->get_timekeeper(0)->set_time_next_pps(time_spec);
    }

    void set_time_unknown_pps(const time_spec_t& time_spec) override
    {
        UHD_LOGGER_INFO("MULTI_USRP") << "    1) catch time transition at pps edge";
        auto end_time                   = std::chrono::steady_clock::now() + 1100ms;
        time_spec_t time_start_last_pps = get_time_last_pps();
        while (time_start_last_pps == get_time_last_pps()) {
            if (std::chrono::steady_clock::now() > end_time) {
                throw uhd::runtime_error("Board 0 may not be getting a PPS signal!\n"
                                         "No PPS detected within the time interval.\n"
                                         "See the application notes for your device.\n");
            }
            std::this_thread::sleep_for(1ms);
        }

        UHD_LOGGER_INFO("MULTI_USRP") << "    2) set times next pps (synchronously)";
        set_time_next_pps(time_spec, ALL_MBOARDS);
        std::this_thread::sleep_for(1s);

        // verify that the time registers are read to be within a few RTT
        for (size_t m = 1; m < get_num_mboards(); m++) {
            time_spec_t time_0 = this->get_time_now(0);
            time_spec_t time_i = this->get_time_now(m);
            // 10 ms: greater than RTT but not too big
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)) {
                UHD_LOGGER_WARNING("MULTI_USRP")
                    << boost::format(
                           "Detected time deviation between board %d and board 0.\n"
                           "Board 0 time is %f seconds.\n"
                           "Board %d time is %f seconds.\n")
                           % m % time_0.get_real_secs() % m % time_i.get_real_secs();
            }
        }
    }

    bool get_time_synchronized(void) override
    {
        for (size_t m = 1; m < get_num_mboards(); m++) {
            time_spec_t time_0 = this->get_time_now(0);
            time_spec_t time_i = this->get_time_now(m);
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)) {
                return false;
            }
        }
        return true;
    }

    void set_command_time(
        const uhd::time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_command_time, time_spec);

        // Set command time on all the blocks that are connected
        for (auto& chain : _rx_chans) {
            if (chain.second.radio->get_block_id().get_device_no() != mboard) {
                continue;
            }
            chain.second.radio->set_command_time(time_spec, chain.second.block_chan);
            if (chain.second.ddc) {
                chain.second.ddc->set_command_time(time_spec, chain.second.block_chan);
            }
        }
        for (auto& chain : _tx_chans) {
            if (chain.second.radio->get_block_id().get_device_no() != mboard) {
                continue;
            }
            chain.second.radio->set_command_time(time_spec, chain.second.block_chan);
            if (chain.second.duc) {
                chain.second.duc->set_command_time(time_spec, chain.second.block_chan);
            }
        }
    }

    void clear_command_time(size_t mboard = ALL_MBOARDS) override
    {
        if (mboard == ALL_MBOARDS) {
            for (size_t i = 0; i < get_num_mboards(); ++i) {
                clear_command_time(i);
            }
            return;
        }

        // Clear command time on all the blocks that are connected
        for (auto& chain : _rx_chans) {
            if (chain.second.radio->get_block_id().get_device_no() != mboard) {
                continue;
            }
            chain.second.radio->clear_command_time(chain.second.block_chan);
            if (chain.second.ddc) {
                chain.second.ddc->clear_command_time(chain.second.block_chan);
            }
        }
        for (auto& chain : _tx_chans) {
            if (chain.second.radio->get_block_id().get_device_no() != mboard) {
                continue;
            }
            chain.second.radio->clear_command_time(chain.second.block_chan);
            if (chain.second.duc) {
                chain.second.duc->clear_command_time(chain.second.block_chan);
            }
        }
    }

    void issue_stream_cmd(
        const stream_cmd_t& stream_cmd, size_t chan = ALL_CHANS) override
    {
        MUX_RX_API_CALL(issue_stream_cmd, stream_cmd);
        auto& rx_chain = _get_rx_chan(chan);
        if (rx_chain.ddc) {
            rx_chain.ddc->issue_stream_cmd(stream_cmd, rx_chain.block_chan);
        } else {
            rx_chain.radio->issue_stream_cmd(stream_cmd, rx_chain.block_chan);
        }
    }

    void set_time_source(
        const std::string& source, const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_time_source, source);
        _get_mbc(mboard)->set_time_source(source);
    }

    std::string get_time_source(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_time_source();
    }

    std::vector<std::string> get_time_sources(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_time_sources();
    }

    void set_clock_source(
        const std::string& source, const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_clock_source, source);
        _get_mbc(mboard)->set_clock_source(source);
    }

    std::string get_clock_source(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_clock_source();
    }

    std::vector<std::string> get_clock_sources(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_clock_sources();
    }

    void set_sync_source(const std::string& clock_source,
        const std::string& time_source,
        const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_sync_source, clock_source, time_source);
        _get_mbc(mboard)->set_sync_source(clock_source, time_source);
    }

    void set_sync_source(
        const device_addr_t& sync_source, const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_sync_source, sync_source);
        _get_mbc(mboard)->set_sync_source(sync_source);
    }

    device_addr_t get_sync_source(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_sync_source();
    }

    std::vector<device_addr_t> get_sync_sources(const size_t mboard) override
    {
        return _get_mbc(mboard)->get_sync_sources();
    }

    void set_clock_source_out(const bool enb, const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_clock_source_out, enb);
        _get_mbc(mboard)->set_clock_source_out(enb);
    }

    void set_time_source_out(const bool enb, const size_t mboard = ALL_MBOARDS) override
    {
        MUX_MB_API_CALL(set_time_source_out, enb);
        _get_mbc(mboard)->set_time_source_out(enb);
    }

    size_t get_num_mboards(void) override
    {
        return _graph->get_num_mboards();
    }

    sensor_value_t get_mboard_sensor(const std::string& name, size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_sensor(name);
    }

    std::vector<std::string> get_mboard_sensor_names(size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_sensor_names();
    }

    // This only works on the USRP2 and B100, both of which are not rfnoc_device
    void set_user_register(const uint8_t, const uint32_t, size_t) override
    {
        throw uhd::not_implemented_error(
            "set_user_register(): Not implemented on this device!");
    }

    // This only works on the B200, which is not an rfnoc_device
    uhd::wb_iface::sptr get_user_settings_iface(const size_t) override
    {
        return nullptr;
    }

    uhd::rfnoc::radio_control& get_radio_control(const size_t chan = 0) override
    {
        return *_get_rx_chan(chan).radio;
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    rx_chan_t _generate_rx_radio_chan(block_id_t radio_id, size_t block_chan)
    {
        auto radio_blk          = _graph->get_block<uhd::rfnoc::radio_control>(radio_id);
        auto radio_source_chain = get_block_chain(_graph, radio_id, block_chan, true);

        // Find out if we have a DDC in the radio block chain
        auto ddc_port_def = [this, radio_source_chain, radio_id, block_chan]() {
            try {
                for (auto edge : radio_source_chain) {
                    if (block_id_t(edge.dst_blockid).match("DDC")) {
                        if (edge.dst_port != block_chan) {
                            /*  We don't expect this to happen very often. But in
                             * the case that port numbers don't match, we need to
                             * disable DDC control to ensure we're not controlling
                             * another channel's DDC
                             */
                            UHD_LOGGER_WARNING("MULTI_USRP")
                                << "DDC in radio chain " << radio_id << ":"
                                << std::to_string(block_chan)
                                << " not connected to the same port number! "
                                   "Disabling DDC control.";
                            break;
                        }
                        auto ddc_blk = _graph->get_block<uhd::rfnoc::ddc_block_control>(
                            edge.dst_blockid);
                        return std::tuple<uhd::rfnoc::ddc_block_control::sptr, size_t>(
                            ddc_blk, block_chan);
                    }
                }
            } catch (const uhd::exception&) {
                UHD_LOGGER_DEBUG("MULTI_USRP")
                    << "No DDC found for radio block " << radio_id << ":"
                    << std::to_string(block_chan);
                // Then just return a nullptr
            }
            return std::tuple<uhd::rfnoc::ddc_block_control::sptr, size_t>(nullptr, 0);
        }();

        // Create the RX chan
        return rx_chan_t(
            {radio_blk, std::get<0>(ddc_port_def), block_chan, radio_source_chain});
    }

    std::vector<rx_chan_t> _generate_mboard_rx_chans(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard)
    {
        // Discover all of the radios on our devices and create a mapping between radio
        // chains and channel numbers
        auto radio_blk_ids = _graph->find_blocks(std::to_string(mboard) + "/Radio");
        // If we don't find any radios, we don't have a multi_usrp object
        if (radio_blk_ids.empty()) {
            throw uhd::runtime_error(
                "[multi_usrp] No radios found in the requested mboard: "
                + std::to_string(mboard));
        }

        // Iterate through the subdev pairs, and try to find a radio that matches
        std::vector<rx_chan_t> new_chans;
        for (auto chan_subdev_pair : spec) {
            bool subdev_found = false;
            for (auto radio_id : radio_blk_ids) {
                auto radio_blk = _graph->get_block<uhd::rfnoc::radio_control>(radio_id);
                size_t block_chan;
                try {
                    block_chan = radio_blk->get_chan_from_dboard_fe(
                        chan_subdev_pair.sd_name, RX_DIRECTION);
                } catch (const uhd::lookup_error&) {
                    // This is OK, since we're probing all radios, this
                    // particular radio may not have the requested frontend name
                    // so it's not one that we want in this list.
                    continue;
                }
                subdev_spec_pair_t radio_subdev(radio_blk->get_slot_name(),
                    radio_blk->get_dboard_fe_from_chan(block_chan, uhd::RX_DIRECTION));
                if (chan_subdev_pair == radio_subdev) {
                    new_chans.push_back(_generate_rx_radio_chan(radio_id, block_chan));
                    subdev_found = true;
                }
            }
            if (!subdev_found) {
                std::string err_msg("Could not find radio on mboard "
                                    + std::to_string(mboard) + " that matches subdev "
                                    + chan_subdev_pair.db_name + ":"
                                    + chan_subdev_pair.sd_name);
                UHD_LOG_ERROR("MULTI_USRP", err_msg);
                throw uhd::lookup_error(err_msg);
            }
        }
        UHD_LOG_TRACE("MULTI_USRP",
            std::string("Using RX subdev " + spec.to_string() + ", found ")
                + std::to_string(new_chans.size()) + " channels for mboard "
                + std::to_string(mboard));
        return new_chans;
    }

    void set_rx_subdev_spec(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard = ALL_MBOARDS) override
    {
        _set_subdev_spec(_rx_chans, [this](size_t current_mboard) {
            return this->get_rx_subdev_spec(current_mboard);
        }, [this](uhd::usrp::subdev_spec_t current_spec, size_t current_mboard) {
            return this->_generate_mboard_rx_chans(current_spec, current_mboard);
        }, [](uhd::rfnoc::graph_edge_t edge) {
            return block_id_t(edge.dst_blockid).match(NODE_ID_SEP);
        }, spec, mboard);
    }

    uhd::usrp::subdev_spec_t get_rx_subdev_spec(size_t mboard = 0) override
    {
        uhd::usrp::subdev_spec_t result;
        for (size_t rx_chan = 0; rx_chan < get_rx_num_channels(); rx_chan++) {
            auto& rx_chain = _rx_chans.at(rx_chan);
            if (rx_chain.radio->get_block_id().get_device_no() == mboard) {
                result.push_back(
                    uhd::usrp::subdev_spec_pair_t(rx_chain.radio->get_slot_name(),
                        rx_chain.radio->get_dboard_fe_from_chan(
                            rx_chain.block_chan, uhd::RX_DIRECTION)));
            }
        }

        return result;
    }

    size_t get_rx_num_channels(void) override
    {
        return _rx_chans.size();
    }

    std::string get_rx_subdev_name(size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_fe_name(rx_chain.block_chan, uhd::RX_DIRECTION);
    }

    void set_rx_rate(double rate, size_t chan = ALL_CHANS) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        MUX_RX_API_CALL(set_rx_rate, rate);
        const double actual_rate = [&]() {
            auto rx_chain = _get_rx_chan(chan);
            if (rx_chain.ddc) {
                return rx_chain.ddc->set_output_rate(rate, rx_chain.block_chan);
            } else {
                return rx_chain.radio->set_rate(rate);
            }
        }();
        if (actual_rate != rate) {
            UHD_LOGGER_WARNING("MULTI_USRP")
                << boost::format(
                       "Could not set RX rate to %.3f MHz. Actual rate is %.3f MHz")
                       % (rate / 1.0e6) % (actual_rate / 1.0e6);
        }
        _rx_rates[chan] = actual_rate;
    }

    void set_rx_spp(const size_t spp, const size_t chan = ALL_CHANS) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        MUX_RX_API_CALL(set_rx_spp, spp);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_property<int>(
            "spp", narrow_cast<int>(spp), rx_chain.block_chan);
    }

    double get_rx_rate(size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        auto& rx_chain = _get_rx_chan(chan);
        if (rx_chain.ddc) {
            return rx_chain.ddc->get_output_rate(rx_chain.block_chan);
        }
        return rx_chain.radio->get_rate();
    }

    meta_range_t get_rx_rates(size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        auto rx_chain = _get_rx_chan(chan);
        if (rx_chain.ddc) {
            return rx_chain.ddc->get_output_rates(rx_chain.block_chan);
        }
        return rx_chain.radio->get_rate_range();
    }

    tune_result_t set_rx_freq(
        const tune_request_t& tune_request, size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);

        // TODO: Add external LO warning

        auto rx_chain = _get_rx_chan(chan);

        rx_chain.radio->set_rx_tune_args(tune_request.args, rx_chain.block_chan);
        //------------------------------------------------------------------
        //-- calculate the tunable frequency ranges of the system
        //------------------------------------------------------------------
        freq_range_t tune_range =
            (rx_chain.ddc)
                ? make_overall_tune_range(
                      rx_chain.radio->get_rx_frequency_range(rx_chain.block_chan),
                      rx_chain.ddc->get_frequency_range(rx_chain.block_chan),
                      rx_chain.radio->get_rx_bandwidth(rx_chain.block_chan))
                : rx_chain.radio->get_rx_frequency_range(rx_chain.block_chan);

        freq_range_t rf_range =
            rx_chain.radio->get_rx_frequency_range(rx_chain.block_chan);
        freq_range_t dsp_range =
            (rx_chain.ddc) ? rx_chain.ddc->get_frequency_range(rx_chain.block_chan)
                           : meta_range_t(0.0, 0.0);
        // Create lambdas to feed to tune_xx_subdev_and_dsp()
        // Note: If there is no DDC present, register empty lambdas for the DSP functions
        auto set_rf_freq = [rx_chain](double freq) {
            rx_chain.radio->set_rx_frequency(freq, rx_chain.block_chan);
        };
        auto get_rf_freq = [rx_chain](void) {
            return rx_chain.radio->get_rx_frequency(rx_chain.block_chan);
        };
        auto set_dsp_freq = [rx_chain](double freq) {
            (rx_chain.ddc) ? rx_chain.ddc->set_freq(freq, rx_chain.block_chan) : 0;
        };
        auto get_dsp_freq = [rx_chain](void) {
            return (rx_chain.ddc) ? rx_chain.ddc->get_freq(rx_chain.block_chan) : 0.0;
        };
        return tune_xx_subdev_and_dsp(RX_SIGN,
            tune_range,
            rf_range,
            dsp_range,
            set_rf_freq,
            get_rf_freq,
            set_dsp_freq,
            get_dsp_freq,
            tune_request);
    }

    double get_rx_freq(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);

        // extract actual dsp and IF frequencies
        const double actual_rf_freq =
            rx_chain.radio->get_rx_frequency(rx_chain.block_chan);
        const double actual_dsp_freq =
            (rx_chain.ddc) ? rx_chain.ddc->get_freq(rx_chain.block_chan) : 0.0;

        return actual_rf_freq - actual_dsp_freq * RX_SIGN;
    }

    freq_range_t get_rx_freq_range(size_t chan = 0) override
    {
        auto fe_freq_range = get_fe_rx_freq_range(chan);

        auto rx_chain = _get_rx_chan(chan);
        uhd::freq_range_t dsp_freq_range =
            (rx_chain.ddc) ? make_overall_tune_range(get_fe_rx_freq_range(chan),
                                 rx_chain.ddc->get_frequency_range(rx_chain.block_chan),
                                 rx_chain.radio->get_rx_bandwidth(rx_chain.block_chan))
                           : get_fe_rx_freq_range(chan);
        return dsp_freq_range;
    }

    freq_range_t get_fe_rx_freq_range(size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_frequency_range(rx_chain.block_chan);
    }

    /**************************************************************************
     * LO controls
     *************************************************************************/
    std::vector<std::string> get_rx_lo_names(size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_names(rx_chain.block_chan);
    }

    void set_rx_lo_source(const std::string& src,
        const std::string& name = ALL_LOS,
        size_t chan             = 0) override
    {
        MUX_RX_API_CALL(set_rx_lo_source, src, name);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_lo_source(src, name, rx_chain.block_chan);
    }

    const std::string get_rx_lo_source(
        const std::string& name = ALL_LOS, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_source(name, rx_chain.block_chan);
    }

    std::vector<std::string> get_rx_lo_sources(
        const std::string& name = ALL_LOS, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_sources(name, rx_chain.block_chan);
    }

    void set_rx_lo_export_enabled(
        bool enabled, const std::string& name = ALL_LOS, size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_lo_export_enabled, enabled, name);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_lo_export_enabled(enabled, name, rx_chain.block_chan);
    }

    bool get_rx_lo_export_enabled(
        const std::string& name = ALL_LOS, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_export_enabled(name, rx_chain.block_chan);
    }

    double set_rx_lo_freq(double freq, const std::string& name, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->set_rx_lo_freq(freq, name, rx_chain.block_chan);
    }

    double get_rx_lo_freq(const std::string& name, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_freq(name, rx_chain.block_chan);
    }

    freq_range_t get_rx_lo_freq_range(const std::string& name, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_lo_freq_range(name, rx_chain.block_chan);
    }

    /*** TX LO API ***/
    std::vector<std::string> get_tx_lo_names(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_names(tx_chain.block_chan);
    }

    void set_tx_lo_source(const std::string& src,
        const std::string& name = ALL_LOS,
        const size_t chan       = 0) override
    {
        MUX_TX_API_CALL(set_tx_lo_source, src, name);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_lo_source(src, name, tx_chain.block_chan);
    }

    const std::string get_tx_lo_source(
        const std::string& name = ALL_LOS, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_source(name, tx_chain.block_chan);
    }

    std::vector<std::string> get_tx_lo_sources(
        const std::string& name = ALL_LOS, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_sources(name, tx_chain.block_chan);
    }

    void set_tx_lo_export_enabled(const bool enabled,
        const std::string& name = ALL_LOS,
        const size_t chan       = 0) override
    {
        MUX_TX_API_CALL(set_tx_lo_export_enabled, enabled, name);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_lo_export_enabled(enabled, name, tx_chain.block_chan);
    }

    bool get_tx_lo_export_enabled(
        const std::string& name = ALL_LOS, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_export_enabled(name, tx_chain.block_chan);
    }

    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->set_tx_lo_freq(freq, name, tx_chain.block_chan);
    }

    double get_tx_lo_freq(const std::string& name, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_freq(name, tx_chain.block_chan);
    }

    freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_lo_freq_range(name, tx_chain.block_chan);
    }

    /**************************************************************************
     * Gain controls
     *************************************************************************/
    void set_rx_gain(double gain, const std::string& name, size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_gain, gain, name);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_gain(gain, name, rx_chain.block_chan);
    }

    std::vector<std::string> get_rx_gain_profile_names(const size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_gain_profile_names(rx_chain.block_chan);
    }

    void set_rx_gain_profile(const std::string& profile, const size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_gain_profile, profile);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_gain_profile(profile, rx_chain.block_chan);
    }

    std::string get_rx_gain_profile(const size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_gain_profile(rx_chain.block_chan);
    }

    void set_normalized_rx_gain(double gain, size_t chan = 0) override
    {
        if (gain > 1.0 || gain < 0.0) {
            throw uhd::runtime_error("Normalized gain out of range, must be in [0, 1].");
        }
        MUX_RX_API_CALL(set_normalized_rx_gain, gain);
        gain_range_t gain_range = get_rx_gain_range(ALL_GAINS, chan);
        double abs_gain =
            (gain * (gain_range.stop() - gain_range.start())) + gain_range.start();
        set_rx_gain(abs_gain, ALL_GAINS, chan);
    }

    void set_rx_agc(bool enable, size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_agc, enable);
        auto& rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_agc(enable, rx_chain.block_chan);
    }

    double get_rx_gain(const std::string& name, size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_gain(name, rx_chain.block_chan);
    }

    double get_normalized_rx_gain(size_t chan = 0) override
    {
        gain_range_t gain_range       = get_rx_gain_range(ALL_GAINS, chan);
        const double gain_range_width = gain_range.stop() - gain_range.start();
        // In case we have a device without a range of gains:
        if (gain_range_width == 0.0) {
            return 0;
        }
        const double norm_gain =
            (get_rx_gain(ALL_GAINS, chan) - gain_range.start()) / gain_range_width;
        // Avoid rounding errors:
        return std::max(std::min(norm_gain, 1.0), 0.0);
    }

    gain_range_t get_rx_gain_range(const std::string& name, size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_gain_range(name, rx_chain.block_chan);
    }

    std::vector<std::string> get_rx_gain_names(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_gain_names(rx_chain.block_chan);
    }

    bool has_rx_power_reference(const size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->has_rx_power_reference(rx_chain.block_chan);
    }

    void set_rx_power_reference(const double power_dbm, const size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_power_reference, power_dbm);
        auto& rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_power_reference(power_dbm, rx_chain.block_chan);
    }

    double get_rx_power_reference(const size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_power_reference(rx_chain.block_chan);
    }

    meta_range_t get_rx_power_range(const size_t chan) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_power_range(rx_chain.block_chan);
    }

    void set_rx_antenna(const std::string& ant, size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_antenna, ant);
        auto& rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_antenna(ant, rx_chain.block_chan);
    }

    std::string get_rx_antenna(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_antenna(rx_chain.block_chan);
    }

    std::vector<std::string> get_rx_antennas(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_antennas(rx_chain.block_chan);
    }

    void set_rx_bandwidth(double bandwidth, size_t chan = 0) override
    {
        MUX_RX_API_CALL(set_rx_bandwidth, bandwidth);
        auto& rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_bandwidth(bandwidth, rx_chain.block_chan);
    }

    double get_rx_bandwidth(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_bandwidth(rx_chain.block_chan);
    }

    meta_range_t get_rx_bandwidth_range(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_bandwidth_range(rx_chain.block_chan);
    }

    dboard_iface::sptr get_rx_dboard_iface(size_t chan = 0) override
    {
        auto& rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_tree()->access<dboard_iface::sptr>("iface").get();
    }

    sensor_value_t get_rx_sensor(const std::string& name, size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_sensor(name, rx_chain.block_chan);
    }

    std::vector<std::string> get_rx_sensor_names(size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_sensor_names(rx_chain.block_chan);
    }

    void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) override
    {
        MUX_RX_API_CALL(set_rx_dc_offset, enb);
        const auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_dc_offset(enb, rx_chain.block_chan);
    }

    void set_rx_dc_offset(
        const std::complex<double>& offset, size_t chan = ALL_CHANS) override
    {
        MUX_RX_API_CALL(set_rx_dc_offset, offset);
        const auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_dc_offset(offset, rx_chain.block_chan);
    }

    meta_range_t get_rx_dc_offset_range(size_t chan = 0) override
    {
        auto rx_chain = _get_rx_chan(chan);
        return rx_chain.radio->get_rx_dc_offset_range(rx_chain.block_chan);
    }

    void set_rx_iq_balance(const bool enb, size_t chan) override
    {
        MUX_RX_API_CALL(set_rx_iq_balance, enb);
        auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_iq_balance(enb, rx_chain.block_chan);
    }

    void set_rx_iq_balance(
        const std::complex<double>& correction, size_t chan = ALL_CHANS) override
    {
        MUX_RX_API_CALL(set_rx_iq_balance, correction);
        const auto rx_chain = _get_rx_chan(chan);
        rx_chain.radio->set_rx_iq_balance(correction, rx_chain.block_chan);
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    tx_chan_t _generate_tx_radio_chan(block_id_t radio_id, size_t block_chan)
    {
        auto radio_blk = _graph->get_block<uhd::rfnoc::radio_control>(radio_id);
        // Now on to the DUC chain
        auto radio_sink_chain = get_block_chain(_graph, radio_id, block_chan, false);

        // Find out if we have a DUC in the radio block chain
        auto duc_port_def = [this, radio_sink_chain, radio_id, block_chan]() {
            try {
                for (auto edge : radio_sink_chain) {
                    if (block_id_t(edge.src_blockid).match("DUC")) {
                        if (edge.src_port != block_chan) {
                            /*  We don't expect this to happen very often. But in
                             * the case that port numbers don't match, we need to
                             * disable DUC control to ensure we're not controlling
                             * another channel's DDC
                             */
                            UHD_LOGGER_WARNING("MULTI_USRP")
                                << "DUC in radio chain " << radio_id << ":"
                                << std::to_string(block_chan)
                                << " not connected to the same port number! "
                                   "Disabling DUC control.";
                            break;
                        }
                        auto duc_blk = _graph->get_block<uhd::rfnoc::duc_block_control>(
                            edge.src_blockid);
                        return std::tuple<uhd::rfnoc::duc_block_control::sptr, size_t>(
                            duc_blk, block_chan);
                    }
                }
            } catch (const uhd::exception&) {
                UHD_LOGGER_DEBUG("MULTI_USRP")
                    << "No DUC found for radio block " << radio_id << ":"
                    << std::to_string(block_chan);
                // Then just return a nullptr
            }
            return std::tuple<uhd::rfnoc::duc_block_control::sptr, size_t>(nullptr, 0);
        }();

        // Create the TX chan
        return tx_chan_t(
            {radio_blk, std::get<0>(duc_port_def), block_chan, radio_sink_chain, replay_config_t()});
    }

    // Generate the TX chains.  Must call with a complete subdev spec for the mboard.
    std::vector<tx_chan_t> _generate_mboard_tx_chans(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard)
    {
        // Discover all of the radios on the device and create a mapping between radio
        // chains and channel numbers
        auto radio_blk_ids = _graph->find_blocks(std::to_string(mboard) + "/Radio");
        // If we don't find any radios, we don't have a multi_usrp object
        if (radio_blk_ids.empty()) {
            throw uhd::runtime_error(
                "[multi_usrp] No radios found in the requested mboard: "
                + std::to_string(mboard));
        }

        // Set up to map Replay blocks and ports to channel in case the user
        // requests Replay buffering on the TX streamer
        const auto replay_blk_ids = _graph->find_blocks(std::to_string(mboard) + "/Replay");
        size_t replay_index = 0;
        size_t replay_port = 0;
        size_t num_replay_ports = 0;
        for (const auto& replay_id : replay_blk_ids) {
            auto replay = _graph->get_block<uhd::rfnoc::replay_block_control>(replay_id);
            num_replay_ports += std::min(
                replay->get_num_input_ports(),
                replay->get_num_output_ports());
        }

        // Iterate through the subdev pairs, and try to find a radio that matches
        std::vector<tx_chan_t> new_chans;
        for (auto chan_subdev_pair : spec) {
            bool subdev_found = false;
            for (auto radio_id : radio_blk_ids) {
                auto radio_blk = _graph->get_block<uhd::rfnoc::radio_control>(radio_id);
                size_t block_chan;
                try {
                    block_chan = radio_blk->get_chan_from_dboard_fe(
                        chan_subdev_pair.sd_name, TX_DIRECTION);
                } catch (const uhd::lookup_error&) {
                    // This is OK, since we're probing all radios, this
                    // particular radio may not have the requested frontend name
                    // so it's not one that we want in this list.
                    continue;
                }
                subdev_spec_pair_t radio_subdev(radio_blk->get_slot_name(),
                    radio_blk->get_dboard_fe_from_chan(block_chan, uhd::TX_DIRECTION));

                if (chan_subdev_pair == radio_subdev) {
                    tx_chan_t tx_chan(_generate_tx_radio_chan(radio_id, block_chan));

                    // Map a Replay block and port to the channel if there are
                    // enough Replay ports to cover all the channels
                    if (num_replay_ports >= spec.size()) {
                        auto replay_id = replay_blk_ids[replay_index];
                        auto replay = _graph->get_block<uhd::rfnoc::replay_block_control>(replay_id);
                        size_t num_ports = std::min(replay->get_num_input_ports(),
                                                    replay->get_num_output_ports());
                        while (replay_port >= num_ports) {
                            // All ports on the current Replay block are allocated.
                            // Get the next Replay block
                            replay_index++;
                            replay_port = 0;
                            replay_id = replay_blk_ids[replay_index];
                            replay = _graph->get_block<uhd::rfnoc::replay_block_control>(replay_id);
                            num_ports = std::min(replay->get_num_input_ports(),
                                                replay->get_num_output_ports());
                        }

                        // Update the Replay configuration (including memory allocation)
                        const auto mem_per_block = replay->get_mem_size() / replay_blk_ids.size();
                        tx_chan.replay.ctrl = replay;
                        tx_chan.replay.port = replay_port;
                        tx_chan.replay.mem_size = mem_per_block / num_ports;
                        tx_chan.replay.start_address = (replay_index * mem_per_block) +
                                                (tx_chan.replay.mem_size * replay_port);

                        // Get the next Replay port
                        replay_port++;
                    } else {
                        tx_chan.replay.ctrl = nullptr;
                    }

                    new_chans.push_back(tx_chan);
                    subdev_found = true;
                }
            }
            if (!subdev_found) {
                std::string err_msg("Could not find radio on mboard "
                                    + std::to_string(mboard) + " that matches subdev "
                                    + chan_subdev_pair.db_name + ":"
                                    + chan_subdev_pair.sd_name);
                UHD_LOG_ERROR("MULTI_USRP", err_msg);
                throw uhd::lookup_error(err_msg);
            }
        }
        UHD_LOG_TRACE("MULTI_USRP",
            std::string("Using TX subdev " + spec.to_string() + ", found ")
                + std::to_string(new_chans.size()) + " channels for mboard "
                + std::to_string(mboard));
        return new_chans;
    }

    void set_tx_subdev_spec(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard = ALL_MBOARDS) override
    {
        _set_subdev_spec(_tx_chans, [this](size_t current_mboard) {
            return this->get_tx_subdev_spec(current_mboard);
        }, [this](uhd::usrp::subdev_spec_t current_spec, size_t current_mboard) {
            return this->_generate_mboard_tx_chans(current_spec, current_mboard);
        }, [](uhd::rfnoc::graph_edge_t edge) {
            return block_id_t(edge.src_blockid).match(NODE_ID_SEP);
        }, spec, mboard);
    }

    uhd::usrp::subdev_spec_t get_tx_subdev_spec(size_t mboard = 0) override
    {
        uhd::usrp::subdev_spec_t result;
        for (size_t tx_chan = 0; tx_chan < get_tx_num_channels(); tx_chan++) {
            auto& tx_chain = _get_tx_chan(tx_chan);
            if (tx_chain.radio->get_block_id().get_device_no() == mboard) {
                result.push_back(
                    uhd::usrp::subdev_spec_pair_t(tx_chain.radio->get_slot_name(),
                        tx_chain.radio->get_dboard_fe_from_chan(
                            tx_chain.block_chan, uhd::TX_DIRECTION)));
            }
        }

        return result;
    }

    size_t get_tx_num_channels(void) override
    {
        return _tx_chans.size();
    }

    std::string get_tx_subdev_name(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_fe_name(tx_chain.block_chan, uhd::TX_DIRECTION);
    }

    void set_tx_rate(double rate, size_t chan = ALL_CHANS) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        MUX_TX_API_CALL(set_tx_rate, rate);
        const double actual_rate = [&]() {
            auto tx_chain = _get_tx_chan(chan);
            if (tx_chain.duc) {
                return tx_chain.duc->set_input_rate(rate, tx_chain.block_chan);
            } else {
                return tx_chain.radio->set_rate(rate);
            }
        }();
        if (actual_rate != rate) {
            UHD_LOGGER_WARNING("MULTI_USRP")
                << boost::format(
                       "Could not set TX rate to %.3f MHz. Actual rate is %.3f MHz")
                       % (rate / 1.0e6) % (actual_rate / 1.0e6);
        }
        _tx_rates[chan] = actual_rate;
    }

    double get_tx_rate(size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        auto& tx_chain = _get_tx_chan(chan);
        if (tx_chain.duc) {
            return tx_chain.duc->get_input_rate(tx_chain.block_chan);
        }
        return tx_chain.radio->get_rate();
    }

    meta_range_t get_tx_rates(size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        auto tx_chain = _get_tx_chan(chan);
        if (tx_chain.duc) {
            return tx_chain.duc->get_input_rates(tx_chain.block_chan);
        }
        return tx_chain.radio->get_rate_range();
    }

    tune_result_t set_tx_freq(
        const tune_request_t& tune_request, size_t chan = 0) override
    {
        std::lock_guard<std::recursive_mutex> l(_graph_mutex);
        auto tx_chain = _get_tx_chan(chan);

        tx_chain.radio->set_tx_tune_args(tune_request.args, tx_chain.block_chan);
        //------------------------------------------------------------------
        //-- calculate the tunable frequency ranges of the system
        //------------------------------------------------------------------
        freq_range_t tune_range =
            (tx_chain.duc)
                ? make_overall_tune_range(
                      tx_chain.radio->get_tx_frequency_range(tx_chain.block_chan),
                      tx_chain.duc->get_frequency_range(tx_chain.block_chan),
                      tx_chain.radio->get_tx_bandwidth(tx_chain.block_chan))
                : tx_chain.radio->get_tx_frequency_range(tx_chain.block_chan);

        freq_range_t rf_range =
            tx_chain.radio->get_tx_frequency_range(tx_chain.block_chan);
        freq_range_t dsp_range =
            (tx_chain.duc) ? tx_chain.duc->get_frequency_range(tx_chain.block_chan)
                           : meta_range_t(0.0, 0.0);
        // Create lambdas to feed to tune_xx_subdev_and_dsp()
        // Note: If there is no DDC present, register empty lambdas for the DSP functions
        auto set_rf_freq = [tx_chain](double freq) {
            tx_chain.radio->set_tx_frequency(freq, tx_chain.block_chan);
        };
        auto get_rf_freq = [tx_chain](void) {
            return tx_chain.radio->get_tx_frequency(tx_chain.block_chan);
        };
        auto set_dsp_freq = [tx_chain](double freq) {
            (tx_chain.duc) ? tx_chain.duc->set_freq(freq, tx_chain.block_chan) : 0;
        };
        auto get_dsp_freq = [tx_chain](void) {
            return (tx_chain.duc) ? tx_chain.duc->get_freq(tx_chain.block_chan) : 0.0;
        };
        return tune_xx_subdev_and_dsp(TX_SIGN,
            tune_range,
            rf_range,
            dsp_range,
            set_rf_freq,
            get_rf_freq,
            set_dsp_freq,
            get_dsp_freq,
            tune_request);
    }

    double get_tx_freq(size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        // extract actual dsp and IF frequencies
        const double actual_rf_freq =
            tx_chain.radio->get_tx_frequency(tx_chain.block_chan);
        const double actual_dsp_freq =
            (tx_chain.duc) ? tx_chain.duc->get_freq(tx_chain.block_chan) : 0.0;

        return actual_rf_freq - actual_dsp_freq * TX_SIGN;
    }

    freq_range_t get_tx_freq_range(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return (tx_chain.duc)
                   ? make_overall_tune_range(get_fe_tx_freq_range(chan),
                         tx_chain.duc->get_frequency_range(tx_chain.block_chan),
                         tx_chain.radio->get_tx_bandwidth(tx_chain.block_chan))
                   : get_fe_tx_freq_range(chan);
    }

    freq_range_t get_fe_tx_freq_range(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_frequency_range(tx_chain.block_chan);
    }

    void set_tx_gain(double gain, const std::string& name, size_t chan = 0) override
    {
        MUX_TX_API_CALL(set_tx_gain, gain, name);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_gain(gain, name, tx_chain.block_chan);
    }

    std::vector<std::string> get_tx_gain_profile_names(const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_gain_profile_names(tx_chain.block_chan);
    }

    void set_tx_gain_profile(const std::string& profile, const size_t chan = 0) override
    {
        MUX_TX_API_CALL(set_tx_gain_profile, profile);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_gain_profile(profile, tx_chain.block_chan);
    }

    std::string get_tx_gain_profile(const size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_gain_profile(tx_chain.block_chan);
    }

    void set_normalized_tx_gain(double gain, size_t chan = 0) override
    {
        if (gain > 1.0 || gain < 0.0) {
            throw uhd::runtime_error("Normalized gain out of range, must be in [0, 1].");
        }
        MUX_TX_API_CALL(set_normalized_tx_gain, gain);
        gain_range_t gain_range = get_tx_gain_range(ALL_GAINS, chan);
        double abs_gain =
            (gain * (gain_range.stop() - gain_range.start())) + gain_range.start();
        set_tx_gain(abs_gain, ALL_GAINS, chan);
    }

    double get_tx_gain(const std::string& name, size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_gain(name, tx_chain.block_chan);
    }

    double get_normalized_tx_gain(size_t chan = 0) override
    {
        gain_range_t gain_range       = get_tx_gain_range(ALL_GAINS, chan);
        const double gain_range_width = gain_range.stop() - gain_range.start();
        // In case we have a device without a range of gains:
        if (gain_range_width == 0.0) {
            return 0;
        }
        const double norm_gain =
            (get_tx_gain(ALL_GAINS, chan) - gain_range.start()) / gain_range_width;
        // Avoid rounding errors:
        return std::max(std::min(norm_gain, 1.0), 0.0);
    }

    gain_range_t get_tx_gain_range(const std::string& name, size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_gain_range(name, tx_chain.block_chan);
    }

    std::vector<std::string> get_tx_gain_names(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_gain_names(tx_chain.block_chan);
    }

    bool has_tx_power_reference(const size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->has_tx_power_reference(tx_chain.block_chan);
    }

    void set_tx_power_reference(const double power_dbm, const size_t chan = 0) override
    {
        MUX_TX_API_CALL(set_tx_power_reference, power_dbm);
        auto& tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_power_reference(power_dbm, tx_chain.block_chan);
    }

    double get_tx_power_reference(const size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_power_reference(tx_chain.block_chan);
    }

    meta_range_t get_tx_power_range(const size_t chan) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_power_range(tx_chain.block_chan);
    }

    void set_tx_antenna(const std::string& ant, size_t chan = 0) override
    {
        MUX_TX_API_CALL(set_tx_antenna, ant);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_antenna(ant, tx_chain.block_chan);
    }

    std::string get_tx_antenna(size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_antenna(tx_chain.block_chan);
    }

    std::vector<std::string> get_tx_antennas(size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_antennas(tx_chain.block_chan);
    }

    void set_tx_bandwidth(double bandwidth, size_t chan = 0) override
    {
        MUX_TX_API_CALL(set_tx_bandwidth, bandwidth);
        auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_bandwidth(bandwidth, tx_chain.block_chan);
    }

    double get_tx_bandwidth(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_bandwidth(tx_chain.block_chan);
    }

    meta_range_t get_tx_bandwidth_range(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_bandwidth_range(tx_chain.block_chan);
    }

    dboard_iface::sptr get_tx_dboard_iface(size_t chan = 0) override
    {
        auto& tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tree()->access<dboard_iface::sptr>("iface").get();
    }

    sensor_value_t get_tx_sensor(const std::string& name, size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_sensor(name, tx_chain.block_chan);
    }

    std::vector<std::string> get_tx_sensor_names(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_sensor_names(tx_chain.block_chan);
    }

    void set_tx_dc_offset(
        const std::complex<double>& offset, size_t chan = ALL_CHANS) override
    {
        MUX_TX_API_CALL(set_tx_dc_offset, offset);
        const auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_dc_offset(offset, tx_chain.block_chan);
    }

    meta_range_t get_tx_dc_offset_range(size_t chan = 0) override
    {
        auto tx_chain = _get_tx_chan(chan);
        return tx_chain.radio->get_tx_dc_offset_range(tx_chain.block_chan);
    }

    void set_tx_iq_balance(
        const std::complex<double>& correction, size_t chan = ALL_CHANS) override
    {
        MUX_TX_API_CALL(set_tx_iq_balance, correction);
        const auto tx_chain = _get_tx_chan(chan);
        tx_chain.radio->set_tx_iq_balance(correction, tx_chain.block_chan);
    }

    /*******************************************************************
     * GPIO methods
     ******************************************************************/
    /*! Helper function to identify the radio and the bank on that radio.
     *
     * Background: Historically, multi_usrp has made up its own GPIO bank names,
     * unrelated to the radios. Now, we need to be a bit more clever to get that
     * legacy behaviour to work.
     *
     * Here's the algorithm:
     * - If the bank ends with 'A' or 'B' we'll use that to identify the radio
     * - Otherwise, we'll pick the first radio
     * - If the bank ends with 'A' or 'B' we strip that suffix
     *
     * The returned radio will now have a GPIO bank with the returned name.
     */
    std::pair<uhd::rfnoc::radio_control::sptr, std::string> _get_gpio_radio_bank(
        const std::string& bank, const size_t mboard)
    {
        UHD_ASSERT_THROW(!bank.empty());
        char suffix = bank[bank.size() - 1];
        std::string slot_name;
        if (suffix == 'A' || suffix == 'a') {
            slot_name = "A";
        }
        if (suffix == 'B' || suffix == 'b') {
            slot_name = "B";
        }

        uhd::rfnoc::radio_control::sptr radio = [bank, mboard, slot_name, this]() {
            for (const auto& radio : _radios[mboard]) {
                if (slot_name.empty() || radio->get_slot_name() == slot_name) {
                    return radio;
                }
            }
            throw uhd::runtime_error(std::string("Could not match GPIO bank ") + bank
                                     + " to a radio block controller.");
        }();

        const std::string normalized_bank = [radio, bank]() {
            auto radio_banks = radio->get_gpio_banks();
            for (auto& radio_bank : radio_banks) {
                if (bank.find(radio_bank) == 0) {
                    return radio_bank;
                }
            }
            throw uhd::runtime_error(std::string("Could not match GPIO bank ") + bank
                                     + " to radio " + radio->get_unique_id());
        }();

        return {radio, normalized_bank};
    }

    std::vector<std::string> get_gpio_banks(const size_t mboard) override
    {
        std::vector<std::string> gpio_banks;
        for (const auto& radio : _radios[mboard]) {
            auto radio_banks = radio->get_gpio_banks();
            for (const auto& bank : radio_banks) {
                gpio_banks.push_back(bank + radio->get_slot_name());
            }
        }

        return gpio_banks;
    }

    void set_gpio_attr(const std::string& bank,
        const std::string& attr,
        const uint32_t value,
        const uint32_t mask = 0xffffffff,
        const size_t mboard = 0) override
    {
        auto radio_bank_pair = _get_gpio_radio_bank(bank, mboard);
        const uint32_t current =
            radio_bank_pair.first->get_gpio_attr(radio_bank_pair.second, attr);
        const uint32_t new_value = (current & ~mask) | (value & mask);
        radio_bank_pair.first->set_gpio_attr(radio_bank_pair.second, attr, new_value);
    }

    uint32_t get_gpio_attr(const std::string& bank,
        const std::string& attr,
        const size_t mboard = 0) override
    {
        auto radio_bank_pair = _get_gpio_radio_bank(bank, mboard);
        return radio_bank_pair.first->get_gpio_attr(radio_bank_pair.second, attr);
    }

    std::vector<std::string> get_gpio_src_banks(const size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_gpio_banks();
    }

    std::vector<std::string> get_gpio_srcs(
        const std::string& bank, const size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_gpio_srcs(bank);
    }

    std::vector<std::string> get_gpio_src(
        const std::string& bank, const size_t mboard = 0) override
    {
        return _get_mbc(mboard)->get_gpio_src(bank);
    }

    void set_gpio_src(const std::string& bank,
        const std::vector<std::string>& src,
        const size_t mboard = 0) override
    {
        _get_mbc(mboard)->set_gpio_src(bank, src);
    }

    /*******************************************************************
     * Filter API methods
     ******************************************************************/
    std::vector<std::string> get_rx_filter_names(const size_t chan) override
    {
        std::vector<std::string> filter_names;
        // Grab the Radio's filters
        auto rx_chan    = _get_rx_chan(chan);
        auto radio_id   = rx_chan.radio->get_block_id();
        auto radio_ctrl = std::dynamic_pointer_cast<detail::filter_node>(rx_chan.radio);
        if (radio_ctrl) {
            auto radio_filters = radio_ctrl->get_rx_filter_names(rx_chan.block_chan);
            // Prepend the radio's block ID to each filter name
            std::transform(radio_filters.begin(),
                radio_filters.end(),
                radio_filters.begin(),
                [radio_id](
                    std::string name) { return radio_id.to_string() + ":" + name; });
            // Add the radio's filter names to the return vector
            filter_names.insert(
                filter_names.end(), radio_filters.begin(), radio_filters.end());
        } else {
            UHD_LOG_DEBUG("MULTI_USRP",
                "Radio block " + radio_id.to_string() + " does not support filters");
        }
        // Grab the DDC's filter
        auto ddc_id   = rx_chan.ddc->get_block_id();
        auto ddc_ctrl = std::dynamic_pointer_cast<detail::filter_node>(rx_chan.ddc);
        if (ddc_ctrl) {
            auto ddc_filters = ddc_ctrl->get_rx_filter_names(rx_chan.block_chan);
            // Prepend the DDC's block ID to each filter name
            std::transform(ddc_filters.begin(),
                ddc_filters.end(),
                ddc_filters.begin(),
                [ddc_id](std::string name) { return ddc_id.to_string() + ":" + name; });
            // Add the radio's filter names to the return vector
            filter_names.insert(
                filter_names.end(), ddc_filters.begin(), ddc_filters.end());
        } else {
            UHD_LOG_DEBUG("MULTI_USRP",
                "DDC block " + ddc_id.to_string() + " does not support filters");
        }
        return filter_names;
    }

    uhd::filter_info_base::sptr get_rx_filter(
        const std::string& name, const size_t chan) override
    {
        try {
            // Get the blockid and filtername separated from the name string
            const auto names        = string::split(name, ":");
            const auto& blockid     = names.first;
            const auto& filter_name = names.second;
            // The block_id_t constructor is pretty smart; let it handle the parsing.
            block_id_t block_id(blockid);
            const auto rx_chan = _get_rx_chan(chan);
            // Try to dynamic cast either the radio or the DDC to a filter_node, and call
            // its filter function
            auto block_ctrl = [rx_chan, block_id, chan]() -> noc_block_base::sptr {
                if (block_id == rx_chan.radio->get_block_id()) {
                    return rx_chan.radio;
                } else if (block_id == rx_chan.ddc->get_block_id()) {
                    return rx_chan.ddc;
                } else {
                    throw uhd::runtime_error("Requested block " + block_id.to_string()
                                             + " does not match block ID in channel "
                                             + std::to_string(chan));
                }
            }();
            auto filter_ctrl = std::dynamic_pointer_cast<detail::filter_node>(block_ctrl);
            if (filter_ctrl) {
                return filter_ctrl->get_rx_filter(filter_name, rx_chan.block_chan);
            }
            std::string err_msg =
                block_ctrl->get_block_id().to_string() + " does not support filters";
            UHD_LOG_ERROR("MULTI_USRP", err_msg);
            throw uhd::runtime_error(err_msg);
        } catch (const uhd::value_error&) {
            // Catch the error from the block_id_t constructor and add better logging
            UHD_LOG_ERROR("MULTI_USRP",
                "Invalid filter name; could not determine block controller from name: "
                    + name);
            throw;
        }
    }

    void set_rx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) override
    {
        MUX_RX_API_CALL(set_rx_filter, name, filter);
        try {
            const auto names        = string::split(name, ":");
            const auto& blockid     = names.first;
            const auto& filter_name = names.second;
            // The block_id_t constructor is pretty smart; let it handle the parsing.
            block_id_t block_id(blockid);
            const auto rx_chan = _get_rx_chan(chan);
            // Try to dynamic cast either the radio or the DDC to a filter_node, and call
            // its filter function
            auto block_ctrl = [rx_chan, block_id, chan]() -> noc_block_base::sptr {
                if (block_id == rx_chan.radio->get_block_id()) {
                    return rx_chan.radio;
                } else if (block_id == rx_chan.ddc->get_block_id()) {
                    return rx_chan.ddc;
                } else {
                    throw uhd::runtime_error("Requested block " + block_id.to_string()
                                             + " does not match block ID in channel "
                                             + std::to_string(chan));
                }
            }();
            auto filter_ctrl = std::dynamic_pointer_cast<detail::filter_node>(block_ctrl);
            if (filter_ctrl) {
                return filter_ctrl->set_rx_filter(
                    filter_name, filter, rx_chan.block_chan);
            }
            std::string err_msg =
                block_ctrl->get_block_id().to_string() + " does not support filters";
            UHD_LOG_ERROR("MULTI_USRP", err_msg);
            throw uhd::runtime_error(err_msg);
        } catch (const uhd::value_error&) {
            // Catch the error from the block_id_t constructor and add better logging
            UHD_LOG_ERROR("MULTI_USRP",
                "Invalid filter name; could not determine block controller from name: "
                    + name);
            throw;
        }
    }

    std::vector<std::string> get_tx_filter_names(const size_t chan) override
    {
        std::vector<std::string> filter_names;
        // Grab the Radio's filters
        auto tx_chan    = _get_tx_chan(chan);
        auto radio_id   = tx_chan.radio->get_block_id();
        auto radio_ctrl = std::dynamic_pointer_cast<detail::filter_node>(tx_chan.radio);
        if (radio_ctrl) {
            auto radio_filters = radio_ctrl->get_tx_filter_names(tx_chan.block_chan);
            // Prepend the radio's block ID to each filter name
            std::transform(radio_filters.begin(),
                radio_filters.end(),
                radio_filters.begin(),
                [radio_id](
                    std::string name) { return radio_id.to_string() + ":" + name; });
            // Add the radio's filter names to the return vector
            filter_names.insert(
                filter_names.end(), radio_filters.begin(), radio_filters.end());
        } else {
            UHD_LOG_DEBUG("MULTI_USRP",
                "Radio block " + radio_id.to_string() + " does not support filters");
        }
        // Grab the DUC's filter
        auto duc_id   = tx_chan.duc->get_block_id();
        auto duc_ctrl = std::dynamic_pointer_cast<detail::filter_node>(tx_chan.duc);
        if (duc_ctrl) {
            auto duc_filters = duc_ctrl->get_tx_filter_names(tx_chan.block_chan);
            // Prepend the DUC's block ID to each filter name
            std::transform(duc_filters.begin(),
                duc_filters.end(),
                duc_filters.begin(),
                [duc_id](std::string name) { return duc_id.to_string() + ":" + name; });
            // Add the radio's filter names to the return vector
            filter_names.insert(
                filter_names.end(), duc_filters.begin(), duc_filters.end());
        } else {
            UHD_LOG_DEBUG("MULTI_USRP",
                "DUC block " + duc_id.to_string() + " does not support filters");
        }
        return filter_names;
    }

    uhd::filter_info_base::sptr get_tx_filter(
        const std::string& name, const size_t chan) override
    {
        try {
            const auto names        = string::split(name, ":");
            const auto& blockid     = names.first;
            const auto& filter_name = names.second;
            // The block_id_t constructor is pretty smart; let it handle the parsing.
            block_id_t block_id(blockid);
            const auto tx_chan = _get_tx_chan(chan);
            // Try to dynamic cast either the radio or the DUC to a filter_node, and call
            // its filter function
            auto block_ctrl = [tx_chan, block_id, chan]() -> noc_block_base::sptr {
                if (block_id == tx_chan.radio->get_block_id()) {
                    return tx_chan.radio;
                } else if (block_id == tx_chan.duc->get_block_id()) {
                    return tx_chan.duc;
                } else {
                    throw uhd::runtime_error("Requested block " + block_id.to_string()
                                             + " does not match block ID in channel "
                                             + std::to_string(chan));
                }
            }();
            auto filter_ctrl = std::dynamic_pointer_cast<detail::filter_node>(block_ctrl);
            if (filter_ctrl) {
                return filter_ctrl->get_tx_filter(filter_name, tx_chan.block_chan);
            }
            std::string err_msg =
                block_ctrl->get_block_id().to_string() + " does not support filters";
            UHD_LOG_ERROR("MULTI_USRP", err_msg);
            throw uhd::runtime_error(err_msg);
        } catch (const uhd::value_error&) {
            // Catch the error from the block_id_t constructor and add better logging
            UHD_LOG_ERROR("MULTI_USRP",
                "Invalid filter name; could not determine block controller from name: "
                    + name);
            throw;
        }
    }

    void set_tx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) override
    {
        MUX_TX_API_CALL(set_tx_filter, name, filter);
        try {
            const auto names        = string::split(name, ":");
            const auto& blockid     = names.first;
            const auto& filter_name = names.second;
            // The block_id_t constructor is pretty smart; let it handle the parsing.
            block_id_t block_id(blockid);
            const auto tx_chan = _get_tx_chan(chan);
            // Try to dynamic cast either the radio or the DUC to a filter_node, and call
            // its filter function
            auto block_ctrl = [tx_chan, block_id, chan]() -> noc_block_base::sptr {
                if (block_id == tx_chan.radio->get_block_id()) {
                    return tx_chan.radio;
                } else if (block_id == tx_chan.duc->get_block_id()) {
                    return tx_chan.duc;
                } else {
                    throw uhd::runtime_error("Requested block " + block_id.to_string()
                                             + " does not match block ID in channel "
                                             + std::to_string(chan));
                }
            }();
            auto filter_ctrl = std::dynamic_pointer_cast<detail::filter_node>(block_ctrl);
            if (filter_ctrl) {
                return filter_ctrl->set_tx_filter(
                    filter_name, filter, tx_chan.block_chan);
            }
            std::string err_msg =
                block_ctrl->get_block_id().to_string() + " does not support filters";
            UHD_LOG_ERROR("MULTI_USRP", err_msg);
            throw uhd::runtime_error(err_msg);
        } catch (const uhd::value_error&) {
            // Catch the error from the block_id_t constructor and add better logging
            UHD_LOG_ERROR("MULTI_USRP",
                "Invalid filter name; could not determine block controller from name: "
                    + name);
            throw;
        }
    }

    mb_controller& get_mb_controller(const size_t mboard) override
    {
        return *_get_mbc(mboard);
    }

private:
    /**************************************************************************
     * Private Helpers
     *************************************************************************/
    mb_controller::sptr _get_mbc(const size_t mb_idx)
    {
        if (mb_idx >= get_num_mboards()) {
            throw uhd::key_error(
                std::string("No such mboard: ") + std::to_string(mb_idx));
        }
        return _graph->get_mb_controller(mb_idx);
    }

    rx_chan_t& _get_rx_chan(const size_t chan)
    {
        if (!_rx_chans.count(chan)) {
            throw uhd::key_error(
                std::string("Invalid RX channel: ") + std::to_string(chan));
        }
        return _rx_chans.at(chan);
    }

    tx_chan_t& _get_tx_chan(const size_t chan)
    {
        if (!_tx_chans.count(chan)) {
            throw uhd::key_error(
                std::string("Invalid TX channel: ") + std::to_string(chan));
        }
        return _tx_chans.at(chan);
    }

    std::vector<graph_edge_t> _connect_rx_chains(std::vector<size_t> chans)
    {
        std::vector<graph_edge_t> edges;
        for (auto chan : chans) {
            UHD_LOG_TRACE(
                "MULTI_USRP", std::string("Connecting RX chain for channel ") + std::to_string(chan));
            auto chain = _rx_chans.at(chan);
            for (auto edge : chain.edge_list) {
                if (block_id_t(edge.dst_blockid).match(NODE_ID_SEP)) {
                    break;
                }
                UHD_LOG_TRACE(
                    "MULTI_USRP", std::string("Connecting RX edge: ") + edge.to_string());
                _graph->connect(
                    edge.src_blockid, edge.src_port, edge.dst_blockid, edge.dst_port);
                edges.push_back(edge);
            }
        }
        return edges;
    }

    std::vector<graph_edge_t> _connect_tx_chain(const size_t chan)
    {
        std::vector<graph_edge_t> edges;

        UHD_LOG_TRACE(
            "MULTI_USRP", std::string("Connecting TX chain for channel ") + std::to_string(chan));
        auto chain = _get_tx_chan(chan);
        for (auto edge : chain.edge_list) {
            edges.push_back(edge);
            if (block_id_t(edge.src_blockid).match(NODE_ID_SEP)) {
                break;
            }
            UHD_LOG_TRACE(
                "MULTI_USRP", std::string("Connecting TX edge: ") + edge.to_string());
            _graph->connect(
                edge.src_blockid, edge.src_port, edge.dst_blockid, edge.dst_port);
        }

        return edges;
    }

    std::vector<graph_edge_t> _connect_tx_chain_with_replay(size_t chan)
    {
        std::vector<graph_edge_t> edges;
        auto tx_chan = _get_tx_chan(chan);
        if (not tx_chan.replay.ctrl) {
            throw uhd::runtime_error(
                "[multi_usrp] No Replay block found to buffer TX stream");
        }
        auto replay_id = tx_chan.replay.ctrl->get_block_id();
        auto replay_port = tx_chan.replay.port;

        // Connect Replay out to Radio in
        try {
            edges = connect_through_blocks(
                _graph,
                replay_id,
                replay_port,
                tx_chan.radio->get_block_id(),
                tx_chan.block_chan);
        } catch (uhd::runtime_error& e) {
            throw uhd::runtime_error(
                std::string("[multi_usrp] Unable to connect Replay block to Radio: ") +
                e.what());
        }

        // Add Replay input edge (for streamer connection)
        auto replay_edges_in = get_block_chain(_graph, replay_id, replay_port, false);
        if (replay_edges_in.size() > 1) {
            throw uhd::runtime_error(
                "[multi_usrp] Unable to connect TX streamer to Replay block: "
                "Unexpected block found before Replay block");
        }
        edges.push_back(replay_edges_in.at(0));

        return edges;
    }

    template<typename ChanType, typename GetSubdevSpecFn, typename GenChansFn, typename CheckEdgeForSepFn>
    void _set_subdev_spec(
        std::unordered_map<size_t, ChanType>& chans,
        GetSubdevSpecFn&& get_subdev_spec,
        GenChansFn&& generate_chans,
        CheckEdgeForSepFn&& check_edge_for_sep,
        const uhd::usrp::subdev_spec_t& spec,
        size_t mboard)
    {
        // First, generate a vector of the channels that we need to register
        auto new_chans = [&]() {
            /* When setting the subdev spec in multiple mboard scenarios, there are two
             * cases we need to handle:
             * 1. Setting all mboard to the same subdev spec. This is the easy case.
             * 2. Setting a single mboard's subdev spec. In this case, we need to update
             * the requested mboard's subdev spec, and keep the old subdev spec for the
             * other mboards.
             */
            std::vector<ChanType> new_chans;
            for (size_t current_mboard = 0; current_mboard < get_num_mboards();
                 ++current_mboard) {
                auto current_spec = [&]() {
                    if (mboard == ALL_MBOARDS || mboard == current_mboard) {
                        // Update all mboards to the same subdev spec OR
                        // only update this mboard to the new subdev spec
                        return spec;
                    } else {
                        // Keep the old subdev spec for this mboard
                        return get_subdev_spec(current_mboard);
                    }
                }();
                auto new_mboard_chans = generate_chans(current_spec, current_mboard);
                new_chans.insert(
                    new_chans.end(), new_mboard_chans.begin(), new_mboard_chans.end());
            }
            return new_chans;
        }();

        // Disconnect and clear existing chains
        UHD_LOG_TRACE("MULTI_USRP", "Disconnecting chains");
        for (auto entry : chans) {
            for (auto edge : entry.second.edge_list) {
                if (check_edge_for_sep(edge)) {
                    break;
                }
                _graph->disconnect(
                    edge.src_blockid, edge.src_port, edge.dst_blockid, edge.dst_port);
            }
        }
        chans.clear();

        // Register new chains
        size_t musrp_channel = 0;
        for (auto chan : new_chans) {
            chans.emplace(musrp_channel++, chan);
        }
    }

    /**************************************************************************
     * Private Attributes
     *************************************************************************/
    //! Devices args used to spawn this multi_usrp
    const uhd::device_addr_t _args;
    //! Reference to rfnoc_graph
    rfnoc_graph::sptr _graph;
    //! Reference to the prop tree
    property_tree::sptr _tree;
    //! Mapping between device number and the radio blocks
    std::unordered_map<size_t, std::vector<uhd::rfnoc::radio_control::sptr>> _radios;
    //! Mapping between channel number and the RFNoC blocks in that RX chain
    std::unordered_map<size_t, rx_chan_t> _rx_chans;
    //! Mapping between channel number and the RFNoC blocks in that TX chain
    std::unordered_map<size_t, tx_chan_t> _tx_chans;
    //! Cache the requested RX rates
    std::unordered_map<size_t, double> _rx_rates;
    //! Cache the requested TX rates
    std::unordered_map<size_t, double> _tx_rates;

    std::recursive_mutex _graph_mutex;

    std::shared_ptr<redirector_device> _device;
};

/******************************************************************************
 * Factory
 *****************************************************************************/
namespace uhd { namespace rfnoc { namespace detail {
// Forward declare
rfnoc_graph::sptr make_rfnoc_graph(
    detail::rfnoc_device::sptr dev, const uhd::device_addr_t& device_addr);

multi_usrp::sptr make_rfnoc_device(
    detail::rfnoc_device::sptr rfnoc_device, const uhd::device_addr_t& dev_addr)
{
    auto graph = uhd::rfnoc::detail::make_rfnoc_graph(rfnoc_device, dev_addr);

    // The serialization of this function is intended to protect against
    // multiple threads instantiating multi_usrp objects for the same
    // device in parallel.
    static std::mutex _map_mutex;
    static std::map<std::weak_ptr<rfnoc_graph>,
        std::weak_ptr<multi_usrp>,
        std::owner_less<std::weak_ptr<rfnoc_graph>>>
        graph_to_musrp;
    multi_usrp::sptr musrp;

    // Check if a multi_usrp was already created for this device
    std::lock_guard<std::mutex> lock(_map_mutex);
    if (graph_to_musrp.count(graph) and not graph_to_musrp[graph].expired()) {
        musrp = graph_to_musrp[graph].lock();
        if (musrp) {
            return musrp;
        }
    }

    // Create a new musrp
    musrp                 = std::make_shared<multi_usrp_rfnoc>(graph, dev_addr);
    graph_to_musrp[graph] = musrp;
    return musrp;
}

}}} // namespace uhd::rfnoc::detail
