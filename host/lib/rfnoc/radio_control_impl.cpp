//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <map>
#include <tuple>

using namespace uhd::rfnoc;

namespace {

const std::string DEFAULT_GAIN_PROFILE("default");

} // namespace

const std::string radio_control::ALL_LOS   = "all";
const std::string radio_control::ALL_GAINS = "";

const uint16_t radio_control_impl::MAJOR_COMPAT = 0;
const uint16_t radio_control_impl::MINOR_COMPAT = 1;

const uhd::fs_path radio_control_impl::DB_PATH("dboard");
const uhd::fs_path radio_control_impl::FE_PATH("frontends");

static constexpr double OVERRUN_RESTART_DELAY = 0.05;

/****************************************************************************
 * Structors
 ***************************************************************************/
radio_control_impl::radio_control_impl(make_args_ptr make_args)
    : radio_control(std::move(make_args))
    , _radio_reg_iface(*this,
          radio_control_impl::regmap::RADIO_BASE_ADDR,
          radio_control_impl::regmap::REG_CHAN_OFFSET)
    , _fpga_compat(regs().peek32(regmap::REG_COMPAT_NUM))
    , _radio_width(regs().peek32(regmap::REG_RADIO_WIDTH))
    , _samp_width(_radio_width >> 16)
    , _spc(_radio_width & 0xFFFF)
    , _last_stream_cmd(
          get_num_output_ports(), uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS)
{
    uhd::assert_fpga_compat(MAJOR_COMPAT,
        MINOR_COMPAT,
        _fpga_compat,
        get_unique_id(),
        get_unique_id(),
        false /* Let it slide if minors mismatch */
    );
    RFNOC_LOG_TRACE(
        "Loading radio with SPC=" << _spc << ", num_inputs=" << get_num_input_ports()
                                  << ", num_outputs=" << get_num_output_ports());
    set_prop_forwarding_policy(forwarding_policy_t::DROP);
    set_action_forwarding_policy(forwarding_policy_t::DROP);
    set_mtu_forwarding_policy(forwarding_policy_t::DROP);
    register_action_handler(ACTION_KEY_STREAM_CMD,
        [this](const res_source_info& src, action_info::sptr action) {
            stream_cmd_action_info::sptr stream_cmd_action =
                std::dynamic_pointer_cast<stream_cmd_action_info>(action);
            if (!stream_cmd_action) {
                RFNOC_LOG_WARNING("Received invalid stream command action!");
                return;
            }
            RFNOC_LOG_TRACE(
                "Received stream command: " << stream_cmd_action->stream_cmd.stream_mode
                                            << " to " << src.to_string());
            if (src.type != res_source_info::OUTPUT_EDGE) {
                RFNOC_LOG_WARNING(
                    "Received stream command, but not to output port! Ignoring.");
                return;
            }
            const size_t port = src.instance;
            if (port >= get_num_output_ports()) {
                RFNOC_LOG_WARNING("Received stream command to invalid output port!");
                return;
            }
            issue_stream_cmd(stream_cmd_action->stream_cmd, port);
        });
    register_action_handler(ACTION_KEY_RX_RESTART_REQ,
        [this](const res_source_info& src, action_info::sptr /*action*/) {
            RFNOC_LOG_TRACE("Received restart request command to " << src.to_string());
            if (src.type != res_source_info::OUTPUT_EDGE) {
                RFNOC_LOG_WARNING(
                    "Received stream command, but not to output port! Ignoring.");
                return;
            }
            auto stream_cmd_action = stream_cmd_action_info::make(
                uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd_action->stream_cmd.stream_now = false;
            stream_cmd_action->stream_cmd.time_spec =
                get_mb_controller()->get_timekeeper(0)->get_time_now()
                + uhd::time_spec_t(OVERRUN_RESTART_DELAY);
            const size_t port = src.instance;
            if (port >= get_num_output_ports()) {
                RFNOC_LOG_WARNING("Received stream command to invalid output port!");
                return;
            }
            post_action({res_source_info::OUTPUT_EDGE, port}, stream_cmd_action);
        });
    // Register spp properties and resolvers
    _spp_prop.reserve(get_num_output_ports());
    _atomic_item_size_in.reserve(get_num_input_ports());
    _atomic_item_size_out.reserve(get_num_output_ports());
    _samp_rate_in.reserve(get_num_input_ports());
    _samp_rate_out.reserve(get_num_output_ports());
    _type_in.reserve(get_num_input_ports());
    _type_out.reserve(get_num_output_ports());
    for (size_t chan = 0; chan < get_num_output_ports(); ++chan) {
        // Default SPP is the maximum value we can fit through the edge, given our MTU
        const int default_spp =
            get_max_spp(get_max_payload_size({res_source_info::OUTPUT_EDGE, chan}));
        _spp_prop.push_back(
            property_t<int>(PROP_KEY_SPP, default_spp, {res_source_info::USER, chan}));
        _atomic_item_size_in.push_back(
            property_t<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
            get_atomic_item_size(),
            {res_source_info::INPUT_EDGE, chan}));
        _atomic_item_size_out.push_back(
            property_t<size_t>(PROP_KEY_ATOMIC_ITEM_SIZE,
            get_atomic_item_size(),
            {res_source_info::OUTPUT_EDGE, chan}));
        _samp_rate_in.push_back(property_t<double>(
            PROP_KEY_SAMP_RATE, get_tick_rate(), {res_source_info::INPUT_EDGE, chan}));
        _samp_rate_out.push_back(property_t<double>(
            PROP_KEY_SAMP_RATE, get_tick_rate(), {res_source_info::OUTPUT_EDGE, chan}));
        _type_in.push_back(property_t<io_type_t>(
            PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, chan}));
        _type_out.push_back(property_t<io_type_t>(
            PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, chan}));

        register_property(&_atomic_item_size_in.back());
        register_property(&_atomic_item_size_out.back());
        register_property(&_spp_prop.back(), [this, chan, &spp = _spp_prop.back()]() {
            const uint32_t words_per_pkt = spp.get() / _spc;
            RFNOC_LOG_TRACE(
                "Setting words_per_pkt to " << words_per_pkt << " on chan " << chan);
            _radio_reg_iface.poke32(
                regmap::REG_RX_MAX_WORDS_PER_PKT, words_per_pkt, chan);
        });
        register_property(&_samp_rate_in.back());
        register_property(&_samp_rate_out.back());
        register_property(&_type_in.back());
        register_property(&_type_out.back());
        // Add AIS resolvers first, they are used as inputs to other resolvers
        add_property_resolver({&_atomic_item_size_in.back(),
            get_mtu_prop_ref({res_source_info::INPUT_EDGE, chan})},
            {&_atomic_item_size_in.back()},
            [this, chan,
                &ais_in = _atomic_item_size_in.back()]() {
                RFNOC_LOG_TRACE("Calling resolver for atomic_item_size in@" << chan);
                ais_in = uhd::math::lcm<size_t>(ais_in, get_atomic_item_size());
                ais_in = std::min<size_t>(ais_in, get_mtu({res_source_info::INPUT_EDGE, chan}));
                if ((ais_in % get_atomic_item_size()) > 0) {
                    ais_in = ais_in - (ais_in % get_atomic_item_size());
                }
                RFNOC_LOG_TRACE("Resolving_atomic_item size in to " << ais_in);
            });
        add_property_resolver({&_atomic_item_size_out.back(),
                                  get_mtu_prop_ref({res_source_info::OUTPUT_EDGE, chan})},
            {&_atomic_item_size_out.back()},
            [this, chan, &ais_out = _atomic_item_size_out.back()]() {
                RFNOC_LOG_TRACE("Calling resolver for atomic_item_size out@" << chan);
                ais_out = uhd::math::lcm<size_t>(ais_out, get_atomic_item_size());
                ais_out = std::min<size_t>(ais_out, get_mtu({res_source_info::OUTPUT_EDGE, chan}));
                if ((ais_out % get_atomic_item_size()) > 0) {
                    ais_out = ais_out - (ais_out % get_atomic_item_size());
                }
                RFNOC_LOG_TRACE("Resolving atomic_item_size out to " << ais_out);
            });
        add_property_resolver({&_spp_prop.back(),
                                  &_atomic_item_size_out.back(),
                                  get_mtu_prop_ref({res_source_info::OUTPUT_EDGE, chan})},
            {&_spp_prop.back()},
            [this,
                chan,
                &spp     = _spp_prop.back(),
                &ais_out = _atomic_item_size_out.back()]() {
                RFNOC_LOG_TRACE("Calling resolver for spp@" << chan);
                const size_t max_pyld =
                    get_max_payload_size({res_source_info::OUTPUT_EDGE, chan});
                const int max_spp = get_max_spp(max_pyld);
                if (spp.get() > max_spp) {
                    RFNOC_LOG_DEBUG("spp value "
                                    << spp.get() << " exceeds MTU of "
                                    << get_mtu({res_source_info::OUTPUT_EDGE, chan})
                                    << "! Coercing to " << max_spp);
                    spp = max_spp;
                }
                if (spp.get() <= 0) {
                    spp = max_spp;
                    RFNOC_LOG_WARNING(
                        "spp must be greater than zero! Coercing to " << spp.get());
                }

                // spp must be a multiple of the output atomic item size divided
                // by bytes-per-sample
                const int spp_multiple = ais_out.get() / (_samp_width / 8);
                if (spp_multiple > max_spp) {
                    RFNOC_LOG_ERROR("Cannot resolve spp! Must be a multiple of "
                                    << spp_multiple << " but max value is " << max_spp);
                    throw uhd::resolve_error("Cannot resolve spp!");
                }
                if (spp.get() % spp_multiple) {
                    if (spp.get() < spp_multiple) {
                        spp = spp_multiple;
                    } else {
                        spp = spp.get() - (spp.get() % spp_multiple);
                    }
                    RFNOC_LOG_WARNING("spp must be a multiple of "
                                      << spp_multiple << "! Coercing to " << spp.get());
                }
                RFNOC_LOG_TRACE("Exiting resolver for spp@"
                                << chan << " (new value: " << spp.get() << ")");
            });
        // Note: The following resolver calls coerce_rate(), which is virtual.
        // At run time, it will use the implementation by the child class.
        add_property_resolver({&_samp_rate_in.back(), &_samp_rate_out.back()},
            {&_samp_rate_in.back(), &_samp_rate_out.back()},
            [this,
                chan,
                &samp_rate_in  = _samp_rate_in.at(chan),
                &samp_rate_out = _samp_rate_out.at(chan)]() {
                RFNOC_LOG_TRACE("Calling resolver for samp_rate@" << chan);
                samp_rate_in  = coerce_rate(samp_rate_in.get());
                samp_rate_out = samp_rate_in.get();
            });
        // Resolvers for type: These are constants
        add_property_resolver({&_type_in.back()},
            {&_type_in.back()},
            [& type_in = _type_in.back()]() { type_in.set(IO_TYPE_SC16); });
        add_property_resolver({&_type_out.back()},
            {&_type_out.back()},
            [& type_out = _type_out.back()]() { type_out.set(IO_TYPE_SC16); });
    }
    // Enable async messages coming from the radio
    const uint32_t xbar_port = 1; // FIXME: Find a better way to figure this out
    RFNOC_LOG_TRACE("Sending async messages to EPID "
                    << regs().get_src_epid() << ", remote port " << regs().get_port_num()
                    << ", xbar port " << xbar_port);
    for (size_t tx_chan = 0; tx_chan < get_num_input_ports(); tx_chan++) {
        // Set the EPID and port of our regs() object (all async messages go to
        // the same location)
        _radio_reg_iface.poke32(
            regmap::REG_TX_ERR_REM_EPID, regs().get_src_epid(), tx_chan);
        _radio_reg_iface.poke32(
            regmap::REG_TX_ERR_REM_PORT, regs().get_port_num(), tx_chan);
        // Set the crossbar port for the async packet routing
        _radio_reg_iface.poke32(regmap::REG_TX_ERR_PORT, xbar_port, tx_chan);
        // Set the async message address
        _radio_reg_iface.poke32(regmap::REG_TX_ERR_ADDR,
            regmap::SWREG_TX_ERR + regmap::SWREG_CHAN_OFFSET * tx_chan,
            tx_chan);
    }
    for (size_t rx_chan = 0; rx_chan < get_num_output_ports(); rx_chan++) {
        // Set the EPID and port of our regs() object (all async messages go to
        // the same location)
        _radio_reg_iface.poke32(
            regmap::REG_RX_ERR_REM_EPID, regs().get_src_epid(), rx_chan);
        _radio_reg_iface.poke32(
            regmap::REG_RX_ERR_REM_PORT, regs().get_port_num(), rx_chan);
        // Set the crossbar port for the async packet routing
        _radio_reg_iface.poke32(regmap::REG_RX_ERR_PORT, xbar_port, rx_chan);
        // Set the async message address
        _radio_reg_iface.poke32(regmap::REG_RX_ERR_ADDR,
            regmap::SWREG_RX_ERR + regmap::SWREG_CHAN_OFFSET * rx_chan,
            rx_chan);
    }
    // Now register a function to receive the async messages
    regs().register_async_msg_validator(
        [this](uint32_t addr, const std::vector<uint32_t>& data) {
            return this->async_message_validator(addr, data);
        });
    regs().register_async_msg_handler([this](uint32_t addr,
                                          const std::vector<uint32_t>& data,
                                          boost::optional<uint64_t> timestamp) {
        this->async_message_handler(addr, data, timestamp);
    });

    // Set the default gain profiles
    _rx_gain_profile_api = std::make_shared<rf_control::default_gain_profile>();
    _tx_gain_profile_api = std::make_shared<rf_control::default_gain_profile>();
} /* ctor */

/******************************************************************************
 * Rate-Related API Calls
 *****************************************************************************/
double radio_control_impl::set_rate(const double rate)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    _rate = rate;
    return rate;
    // FIXME:
    ////_tick_rate = rate;
    ////_time64->set_tick_rate(_tick_rate);
    ////_time64->self_test();
    //// set_command_tick_rate(rate);
}

double radio_control_impl::get_rate() const
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rate;
}

uhd::meta_range_t radio_control_impl::get_rate_range() const
{
    RFNOC_LOG_TRACE("Using default implementation of get_rx_rate_range()");
    uhd::meta_range_t result;
    result.push_back(get_rate());
    return result;
}

size_t radio_control_impl::get_spc() const
{
    return _spc;
}


/******************************************************************************
 * Time-Related API Calls
 *****************************************************************************/
uint64_t radio_control_impl::get_ticks_now()
{
    // Time registers added in 0.1
    if (_fpga_compat < 1) {
        return get_mb_controller()->get_timekeeper(0)->get_ticks_now();
    }
    // Applying the command time here allows for testing of timed commands,
    // but all register accesses should use the command time by default.
    return regs().peek64(regmap::REG_TIME_LO, get_command_time(0));
}

uhd::time_spec_t radio_control_impl::get_time_now()
{
    return uhd::time_spec_t::from_ticks(get_ticks_now(), get_rate());
}

/****************************************************************************
 * RF API
 ***************************************************************************/
void radio_control_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    _tx_antenna[chan] = ant;
}

void radio_control_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    _rx_antenna[chan] = ant;
}

double radio_control_impl::set_tx_frequency(const double freq, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_freq[chan] = freq;
}

void radio_control_impl::set_tx_tune_args(const uhd::device_addr_t&, const size_t)
{
    RFNOC_LOG_TRACE("tune_args not supported by this radio.");
}

double radio_control_impl::set_rx_frequency(const double freq, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_freq[chan] = freq;
}

void radio_control_impl::set_rx_tune_args(const uhd::device_addr_t&, const size_t)
{
    RFNOC_LOG_TRACE("tune_args not supported by this radio.");
}

std::vector<std::string> radio_control_impl::get_tx_gain_names(const size_t) const
{
    return {ALL_GAINS};
}

std::vector<std::string> radio_control_impl::get_rx_gain_names(const size_t) const
{
    return {ALL_GAINS};
}

uhd::gain_range_t radio_control_impl::get_tx_gain_range(const size_t chan) const
{
    RFNOC_LOG_DEBUG("Using default implementation of get_tx_gain_range()");
    uhd::gain_range_t result;
    std::lock_guard<std::mutex> l(_cache_mutex);
    result.push_back(_rx_gain.at(chan));
    return result;
}

uhd::gain_range_t radio_control_impl::get_tx_gain_range(
    const std::string& name, const size_t chan) const
{
    if (name != ALL_GAINS) {
        throw uhd::value_error(
            std::string("get_tx_gain_range(): Unknown gain name `") + name + "'!");
    }
    return get_tx_gain_range(chan);
}

uhd::gain_range_t radio_control_impl::get_rx_gain_range(const size_t chan) const
{
    RFNOC_LOG_DEBUG("Using default implementation of get_rx_gain_range()");
    uhd::gain_range_t result;
    std::lock_guard<std::mutex> l(_cache_mutex);
    result.push_back(_rx_gain.at(chan));
    return result;
}

uhd::gain_range_t radio_control_impl::get_rx_gain_range(
    const std::string& name, const size_t chan) const
{
    if (name != ALL_GAINS) {
        throw uhd::value_error(
            std::string("get_rx_gain_range(): Unknown gain name `") + name + "'!");
    }
    return get_rx_gain_range(chan);
}

double radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    _tx_gain[chan] = gain;
    return gain;
}

double radio_control_impl::set_tx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    if (name != ALL_GAINS) {
        throw uhd::key_error(
            std::string("set_tx_gain(): Gain name `") + name + "' is not defined!");
    }
    return set_tx_gain(gain, chan);
}

double radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    _rx_gain[chan] = gain;
    return gain;
}

double radio_control_impl::set_rx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    if (name != ALL_GAINS) {
        throw uhd::key_error(
            std::string("set_rx_gain(): Gain name `") + name + "' is not defined!");
    }
    return set_rx_gain(gain, chan);
}

bool radio_control_impl::has_rx_power_reference(const size_t chan)
{
    return _rx_pwr_mgr.empty() ? false : _rx_pwr_mgr.at(chan)->has_power_data();
}

bool radio_control_impl::has_tx_power_reference(const size_t chan)
{
    return _tx_pwr_mgr.empty() ? false : _tx_pwr_mgr.at(chan)->has_power_data();
}

void radio_control_impl::set_rx_power_reference(const double power_dbm, const size_t chan)
{
    if (_rx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "set_rx_power_reference() is not supported on this radio!");
    }
    _rx_pwr_mgr.at(chan)->set_power(power_dbm);
}

void radio_control_impl::set_tx_power_reference(const double power_dbm, const size_t chan)
{
    if (_tx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "set_tx_power_reference() is not supported on this radio!");
    }
    _tx_pwr_mgr.at(chan)->set_power(power_dbm);
}

void radio_control_impl::set_rx_agc(const bool, const size_t)
{
    throw uhd::not_implemented_error("set_rx_agc() is not supported on this radio!");
}

void radio_control_impl::set_tx_gain_profile(const std::string& profile, const size_t chan)
{
    _tx_gain_profile_api->set_gain_profile(profile, chan);
}

void radio_control_impl::set_rx_gain_profile(const std::string& profile, const size_t chan)
{
    _rx_gain_profile_api->set_gain_profile(profile, chan);
}

std::vector<std::string> radio_control_impl::get_tx_gain_profile_names(const size_t chan) const
{
    return _tx_gain_profile_api->get_gain_profile_names(chan);
}

std::vector<std::string> radio_control_impl::get_rx_gain_profile_names(const size_t chan) const
{
    return _rx_gain_profile_api->get_gain_profile_names(chan);
}

std::string radio_control_impl::get_tx_gain_profile(const size_t chan) const
{
    return _tx_gain_profile_api->get_gain_profile(chan);
}

std::string radio_control_impl::get_rx_gain_profile(const size_t chan) const
{
    return _rx_gain_profile_api->get_gain_profile(chan);
}

double radio_control_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_bandwidth[chan] = bandwidth;
}

double radio_control_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_bandwidth[chan] = bandwidth;
}

std::string radio_control_impl::get_tx_antenna(const size_t chan) const
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_antenna.at(chan);
}

std::string radio_control_impl::get_rx_antenna(const size_t chan) const
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_antenna.at(chan);
}

std::vector<std::string> radio_control_impl::get_tx_antennas(const size_t chan) const
{
    RFNOC_LOG_DEBUG("get_tx_antennas(): Using default implementation.");
    std::lock_guard<std::mutex> l(_cache_mutex);
    return {_tx_antenna.at(chan)};
}

std::vector<std::string> radio_control_impl::get_rx_antennas(const size_t chan) const
{
    RFNOC_LOG_DEBUG("get_rx_antennas(): Using default implementation.");
    std::lock_guard<std::mutex> l(_cache_mutex);
    return {_rx_antenna.at(chan)};
}

double radio_control_impl::get_tx_frequency(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_freq.at(chan);
}

double radio_control_impl::get_rx_frequency(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_freq.at(chan);
}

uhd::freq_range_t radio_control_impl::get_tx_frequency_range(const size_t) const
{
    RFNOC_LOG_WARNING(
        "get_tx_frequency_range() not implemented! Returning current rate only.");
    uhd::freq_range_t result;
    result.push_back(get_rate());
    return result;
}

uhd::freq_range_t radio_control_impl::get_rx_frequency_range(const size_t) const
{
    RFNOC_LOG_WARNING(
        "get_rx_frequency_range() not implemented! Returning current rate only.");
    uhd::freq_range_t result;
    result.push_back(get_rate());
    return result;
}

double radio_control_impl::get_tx_gain(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_gain.at(chan);
}

double radio_control_impl::get_rx_gain(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_gain.at(chan);
}

double radio_control_impl::get_tx_gain(const std::string& name, const size_t chan)
{
    if (name != ALL_GAINS) {
        throw uhd::value_error(
            std::string("get_tx_gain(): Unknown gain name `") + name + "'");
    }
    return get_tx_gain(chan);
}

double radio_control_impl::get_rx_gain(const std::string& name, const size_t chan)
{
    if (name != ALL_GAINS) {
        throw uhd::value_error(
            std::string("get_rx_gain(): Unknown gain name `") + name + "'");
    }
    return get_rx_gain(chan);
}

double radio_control_impl::get_tx_bandwidth(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _tx_bandwidth.at(chan);
}

double radio_control_impl::get_rx_bandwidth(const size_t chan)
{
    std::lock_guard<std::mutex> l(_cache_mutex);
    return _rx_bandwidth.at(chan);
}

uhd::meta_range_t radio_control_impl::get_tx_bandwidth_range(size_t chan) const
{
    RFNOC_LOG_DEBUG("get_tx_bandwidth_range(): Using default implementation.");
    uhd::meta_range_t result;
    std::lock_guard<std::mutex> l(_cache_mutex);
    result.push_back(_rx_bandwidth.at(chan));
    return result;
}

uhd::meta_range_t radio_control_impl::get_rx_bandwidth_range(size_t chan) const
{
    RFNOC_LOG_DEBUG("get_tx_bandwidth_range(): Using default implementation.");
    uhd::meta_range_t result;
    std::lock_guard<std::mutex> l(_cache_mutex);
    result.push_back(_rx_bandwidth.at(chan));
    return result;
}

double radio_control_impl::get_rx_power_reference(const size_t chan)
{
    if (_rx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "get_rx_power_reference() is not supported on this radio!");
    }
    return _rx_pwr_mgr.at(chan)->get_power();
}

double radio_control_impl::get_tx_power_reference(const size_t chan)
{
    if (_tx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "get_tx_power_reference() is not supported on this radio!");
    }
    return _tx_pwr_mgr.at(chan)->get_power();
}

std::vector<std::string> radio_control_impl::get_rx_power_ref_keys(const size_t chan)
{
    if (_rx_pwr_mgr.empty()) {
        return {};
    }
    return {_rx_pwr_mgr.at(chan)->get_key(), _rx_pwr_mgr.at(chan)->get_serial()};
}

std::vector<std::string> radio_control_impl::get_tx_power_ref_keys(const size_t chan)
{
    if (_tx_pwr_mgr.empty()) {
        return {};
    }
    return {_tx_pwr_mgr.at(chan)->get_key(), _tx_pwr_mgr.at(chan)->get_serial()};
}

uhd::meta_range_t radio_control_impl::get_rx_power_range(const size_t chan)
{
    if (_rx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "get_rx_power_range() is not supported on this radio!");
    }
    return _rx_pwr_mgr.at(chan)->get_power_range();
}

uhd::meta_range_t radio_control_impl::get_tx_power_range(const size_t chan)
{
    if (_tx_pwr_mgr.empty()) {
        throw uhd::not_implemented_error(
            "get_tx_power_range() is not supported on this radio!");
    }
    return _tx_pwr_mgr.at(chan)->get_power_range();
}


/******************************************************************************
 * LO Default API
 *****************************************************************************/
std::vector<std::string> radio_control_impl::get_rx_lo_names(const size_t) const
{
    return {};
}

std::vector<std::string> radio_control_impl::get_rx_lo_sources(
    const std::string&, const size_t) const
{
    return {"internal"};
}

uhd::freq_range_t radio_control_impl::get_rx_lo_freq_range(
    const std::string&, const size_t) const
{
    return uhd::freq_range_t();
}

void radio_control_impl::set_rx_lo_source(
    const std::string&, const std::string&, const size_t)
{
    throw uhd::not_implemented_error("set_rx_lo_source is not supported on this radio");
}

const std::string radio_control_impl::get_rx_lo_source(const std::string&, const size_t)
{
    return "internal";
}

void radio_control_impl::set_rx_lo_export_enabled(bool, const std::string&, const size_t)
{
    throw uhd::not_implemented_error(
        "set_rx_lo_export_enabled is not supported on this radio");
}

bool radio_control_impl::get_rx_lo_export_enabled(const std::string&, const size_t)
{
    return false;
}
double radio_control_impl::set_rx_lo_freq(double, const std::string&, const size_t)
{
    throw uhd::not_implemented_error("set_rx_lo_freq is not supported on this radio");
}

double radio_control_impl::get_rx_lo_freq(const std::string&, const size_t chan)
{
    return get_rx_frequency(chan);
}

std::vector<std::string> radio_control_impl::get_tx_lo_names(const size_t) const
{
    return {};
}

std::vector<std::string> radio_control_impl::get_tx_lo_sources(
    const std::string&, const size_t) const
{
    return {"internal"};
}

uhd::freq_range_t radio_control_impl::get_tx_lo_freq_range(
    const std::string&, const size_t)
{
    return uhd::freq_range_t();
}

void radio_control_impl::set_tx_lo_source(
    const std::string&, const std::string&, const size_t)
{
    throw uhd::not_implemented_error("set_tx_lo_source is not supported on this radio");
}
const std::string radio_control_impl::get_tx_lo_source(const std::string&, const size_t)
{
    return "internal";
}

void radio_control_impl::set_tx_lo_export_enabled(
    const bool, const std::string&, const size_t)
{
    throw uhd::not_implemented_error(
        "set_tx_lo_export_enabled is not supported on this radio");
}

bool radio_control_impl::get_tx_lo_export_enabled(const std::string&, const size_t)
{
    return false;
}

double radio_control_impl::set_tx_lo_freq(const double, const std::string&, const size_t)
{
    throw uhd::not_implemented_error("set_tx_lo_freq is not supported on this radio");
}

double radio_control_impl::get_tx_lo_freq(const std::string&, const size_t chan)
{
    return get_tx_frequency(chan);
}

/******************************************************************************
 * Calibration-Related API Calls
 *****************************************************************************/
void radio_control_impl::set_tx_dc_offset(const std::complex<double>&, size_t)
{
    throw uhd::not_implemented_error("set_tx_dc_offset() is not supported on this radio");
}

uhd::meta_range_t radio_control_impl::get_tx_dc_offset_range(size_t) const
{
    return uhd::meta_range_t(0.0, 0.0);
}

void radio_control_impl::set_tx_iq_balance(const std::complex<double>&, size_t)
{
    throw uhd::not_implemented_error(
        "set_tx_iq_balance() is not supported on this radio");
}

void radio_control_impl::set_rx_dc_offset(const bool enb, size_t)
{
    RFNOC_LOG_DEBUG("set_rx_dc_offset() has no effect on this radio");
    if (enb) {
        throw uhd::not_implemented_error(
            "set_rx_dc_offset(bool) is not supported on this radio");
    }
}

void radio_control_impl::set_rx_dc_offset(const std::complex<double>&, size_t)
{
    throw uhd::not_implemented_error(
        "set_rx_dc_offset(complex) is not supported on this radio");
}

uhd::meta_range_t radio_control_impl::get_rx_dc_offset_range(size_t) const
{
    return uhd::meta_range_t(0.0, 0.0);
}

void radio_control_impl::set_rx_iq_balance(const bool enb, size_t)
{
    RFNOC_LOG_DEBUG("set_rx_iq_balance() has no effect on this radio");
    if (enb) {
        throw uhd::not_implemented_error(
            "set_rx_iq_balance(bool) is not supported on this radio");
    }
}

void radio_control_impl::set_rx_iq_balance(const std::complex<double>&, size_t)
{
    throw uhd::not_implemented_error(
        "set_rx_iq_balance() is not supported on this radio");
}

/******************************************************************************
 * GPIO Controls
 *****************************************************************************/
std::vector<std::string> radio_control_impl::get_gpio_banks() const
{
    return {};
}

void radio_control_impl::set_gpio_attr(
    const std::string&, const std::string&, const uint32_t)
{
    throw uhd::not_implemented_error("set_gpio_attr() not implemented on this radio!");
}

uint32_t radio_control_impl::get_gpio_attr(const std::string&, const std::string&)
{
    throw uhd::not_implemented_error("get_gpio_attr() not implemented on this radio!");
}

/**************************************************************************
 * Sensor API
 *************************************************************************/
std::vector<std::string> radio_control_impl::get_rx_sensor_names(size_t) const
{
    return {};
}

uhd::sensor_value_t radio_control_impl::get_rx_sensor(const std::string& name, size_t)
{
    throw uhd::key_error(std::string("Unknown RX sensor: ") + name);
}

std::vector<std::string> radio_control_impl::get_tx_sensor_names(size_t) const
{
    return {};
}

uhd::sensor_value_t radio_control_impl::get_tx_sensor(const std::string& name, size_t)
{
    throw uhd::key_error(std::string("Unknown TX sensor: ") + name);
}

/**************************************************************************
 * EEPROM API
 *************************************************************************/
void radio_control_impl::set_db_eeprom(const uhd::eeprom_map_t&)
{
    throw uhd::not_implemented_error("set_db_eeprom() not implemented for this radio!");
}

uhd::eeprom_map_t radio_control_impl::get_db_eeprom()
{
    return {};
}

/****************************************************************************
 * Streaming API
 ***************************************************************************/
void radio_control_impl::issue_stream_cmd(
    const uhd::stream_cmd_t& stream_cmd, const size_t chan)
{
    // std::lock_guard<std::mutex> lock(_mutex);
    RFNOC_LOG_TRACE("radio_control_impl::issue_stream_cmd(chan="
                    << chan << ", mode=" << char(stream_cmd.stream_mode) << ")");
    _last_stream_cmd[chan] = stream_cmd;

    // calculate the command word
    const std::unordered_map<stream_cmd_t::stream_mode_t, uint32_t, std::hash<size_t>>
        stream_mode_to_cmd_word{
            {stream_cmd_t::STREAM_MODE_START_CONTINUOUS, regmap::RX_CMD_CONTINUOUS},
            {stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS, regmap::RX_CMD_STOP},
            {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, regmap::RX_CMD_FINITE},
            {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, regmap::RX_CMD_FINITE}};
    const uint32_t cmd_bits = stream_mode_to_cmd_word.at(stream_cmd.stream_mode);
    const uint32_t cmd_word =
        cmd_bits
        | (uint32_t((stream_cmd.stream_now) ? 0 : 1) << regmap::RX_CMD_TIMED_POS);

    if (cmd_bits == regmap::RX_CMD_FINITE) {
        if (stream_cmd.num_samps == 0) {
            RFNOC_LOG_WARNING("Ignoring stream command for finite acquisition of "
                              "zero samples");
            return;
        }
        uint64_t num_words = stream_cmd.num_samps / _spc;
        constexpr uint64_t max_num_words = 0x00FFFFFFFFFFFF; // 48 bits
        if (stream_cmd.num_samps % _spc != 0) {
            num_words++;
            RFNOC_LOG_WARNING("The requested "
                + std::to_string(stream_cmd.num_samps)
                + " samples is not a multiple of the samples per cycle ("
                + std::to_string(_spc) + "); returning "
                + std::to_string(num_words * _spc) + " samples.");
        }
        if (num_words > max_num_words) {
            RFNOC_LOG_ERROR("Requesting too many samples in a single burst! "
                            "Requested "
                            + std::to_string(stream_cmd.num_samps)
                            + ", maximum "
                              "is "
                            + std::to_string(max_num_words) + "."); // FIXME
            RFNOC_LOG_INFO(
                "Note that a decimation block will increase the number of samples "
                "per burst by the decimation factor. Your application may have "
                "requested fewer samples.");
            throw uhd::value_error("Requested too many samples in a single burst.");
        }
        _radio_reg_iface.poke32(
            regmap::REG_RX_CMD_NUM_WORDS_HI, uint32_t(num_words >> 32), chan);
        _radio_reg_iface.poke32(
            regmap::REG_RX_CMD_NUM_WORDS_LO, uint32_t(num_words & 0xFFFFFFFF), chan);
    }
    if (!stream_cmd.stream_now) {
        const uint64_t ticks = stream_cmd.time_spec.to_ticks(get_tick_rate());
        _radio_reg_iface.poke32(regmap::REG_RX_CMD_TIME_HI, uint32_t(ticks >> 32), chan);
        _radio_reg_iface.poke32(regmap::REG_RX_CMD_TIME_LO, uint32_t(ticks >> 0), chan);
    }
    _radio_reg_iface.poke32(regmap::REG_RX_CMD, cmd_word, chan);
}

void radio_control_impl::enable_rx_timestamps(const bool enable, const size_t chan)
{
    _radio_reg_iface.poke32(regmap::REG_RX_HAS_TIME, enable ? 0x1 : 0x0, chan);
}

/******************************************************************************
 * Private methods
 *****************************************************************************/
bool radio_control_impl::async_message_validator(
    uint32_t addr, const std::vector<uint32_t>& data)
{
    if (data.empty()) {
        return false;
    }
    // For these calculations, see below
    const uint32_t addr_base = (addr >= regmap::SWREG_RX_ERR) ? regmap::SWREG_RX_ERR
                                                              : regmap::SWREG_TX_ERR;
    const uint32_t chan        = (addr - addr_base) / regmap::SWREG_CHAN_OFFSET;
    const uint32_t addr_offset = addr % regmap::SWREG_CHAN_OFFSET;
    const uint32_t code        = data[0];
    if (addr_offset > 0) {
        return false;
    }
    if (addr_base == regmap::SWREG_RX_ERR) {
        if (chan >= get_num_output_ports()) {
            return false;
        }
        switch (code) {
            case err_codes::ERR_RX_OVERRUN:
                return true;
            case err_codes::ERR_RX_LATE_CMD:
                return true;
            default:
                return false;
        }
    }
    if (addr_base == regmap::SWREG_TX_ERR) {
        if (chan >= get_num_input_ports()) {
            return false;
        }
        switch (code) {
            case err_codes::ERR_TX_UNDERRUN:
                return true;
            case err_codes::ERR_TX_LATE_DATA:
                return true;
            case err_codes::EVENT_TX_BURST_ACK:
                return true;
            default:
                return false;
        }
    }
    return false;
}

void radio_control_impl::async_message_handler(
    uint32_t addr, const std::vector<uint32_t>& data, boost::optional<uint64_t> timestamp)
{
    if (data.empty()) {
        RFNOC_LOG_WARNING(
            str(boost::format("Received async message with invalid length %d!")
                % data.size()));
        return;
    }
    if (data.size() > 1) {
        RFNOC_LOG_WARNING(
            str(boost::format("Received async message with extra data, length %d!")
                % data.size()));
    }
    // Reminder: The address is calculated as:
    // BASE + 64 * chan + addr_offset
    // BASE == 0x0000 for TX, 0x1000 for RX
    const uint32_t addr_base = (addr >= regmap::SWREG_RX_ERR) ? regmap::SWREG_RX_ERR
                                                              : regmap::SWREG_TX_ERR;
    const uint32_t chan = (addr - addr_base) / regmap::SWREG_CHAN_OFFSET;
    // Note: addr_offset is always going to be zero for now, because we only
    // have one "register" that gets hit for either RX or TX, but we'll keep it
    // in case we add other regs in the future
    const uint32_t addr_offset = addr % regmap::SWREG_CHAN_OFFSET;
    const uint32_t code        = data[0];
    RFNOC_LOG_TRACE(
        str(boost::format("Received async message to addr 0x%08X, data length %d words, "
                          "%s channel %d, addr_offset %d, has timestamp %d")
            % addr % data.size() % (addr_base == regmap::SWREG_TX_ERR ? "TX" : "RX")
            % chan % addr_offset % int(bool(timestamp))));
    if (timestamp) {
        RFNOC_LOG_TRACE(
            str(boost::format("Async message timestamp: %ul") % timestamp.get()));
    }
    switch (addr_base + addr_offset) {
        case regmap::SWREG_TX_ERR: {
            if (chan >= get_num_input_ports()) {
                RFNOC_LOG_WARNING(
                    "Cannot process TX-related async message to invalid chan " << chan);
                return;
            }
            switch (code) {
                case err_codes::ERR_TX_UNDERRUN: {
                    auto tx_event_action = tx_event_action_info::make(
                        uhd::async_metadata_t::EVENT_CODE_UNDERFLOW, timestamp);
                    post_action(res_source_info{res_source_info::INPUT_EDGE, chan},
                        tx_event_action);
                    UHD_LOG_FASTPATH("U");
                    RFNOC_LOG_TRACE("Posting underrun event action message.");
                    break;
                }
                case err_codes::ERR_TX_LATE_DATA: {
                    auto tx_event_action = tx_event_action_info::make(
                        uhd::async_metadata_t::EVENT_CODE_TIME_ERROR, timestamp);
                    post_action(res_source_info{res_source_info::INPUT_EDGE, chan},
                        tx_event_action);
                    UHD_LOG_FASTPATH("L");
                    RFNOC_LOG_TRACE("Posting late data event action message.");
                    break;
                }
                case err_codes::EVENT_TX_BURST_ACK: {
                    auto tx_event_action = tx_event_action_info::make(
                        uhd::async_metadata_t::EVENT_CODE_BURST_ACK, timestamp);
                    post_action(res_source_info{res_source_info::INPUT_EDGE, chan},
                        tx_event_action);
                    RFNOC_LOG_TRACE("Posting burst ack event action message.");
                    break;
                }
            }
            break;
        }
        case regmap::SWREG_RX_ERR: {
            if (chan >= get_num_output_ports()) {
                RFNOC_LOG_WARNING(
                    "Cannot process RX-related async message to invalid chan " << chan);
                return;
            }
            switch (code) {
                case err_codes::ERR_RX_OVERRUN: {
                    UHD_LOG_FASTPATH("O");
                    auto rx_event_action = rx_event_action_info::make(
                        uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
                    const bool cont_mode = _last_stream_cmd.at(chan).stream_mode
                                           == stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
                    rx_event_action->args["cont_mode"] = std::to_string(cont_mode);
                    RFNOC_LOG_TRACE("Posting overrun event action message.");
                    post_action(res_source_info{res_source_info::OUTPUT_EDGE, chan},
                        rx_event_action);
                    break;
                }
                case err_codes::ERR_RX_LATE_CMD:
                    UHD_LOG_FASTPATH("L");
                    auto rx_event_action = rx_event_action_info::make(
                        uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND);
                    RFNOC_LOG_TRACE("Posting RX late command message.");
                    post_action(res_source_info{res_source_info::OUTPUT_EDGE, chan},
                        rx_event_action);
                    break;
            }
            break;
        }
        default:
            RFNOC_LOG_WARNING(str(
                boost::format("Received async message to invalid addr 0x%08X!") % addr));
    }
}

int radio_control_impl::get_max_spp(const size_t bytes)
{
    const int packet_samps = bytes / (_samp_width / 8);
    return packet_samps - (packet_samps % _spc);
}
