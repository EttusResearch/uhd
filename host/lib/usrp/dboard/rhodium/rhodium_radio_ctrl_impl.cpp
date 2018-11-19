//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_radio_ctrl_impl.hpp"
#include "rhodium_constants.hpp"
#include <uhdlib/utils/narrow.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <cmath>
#include <cstdlib>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {
    constexpr char RX_FE_CONNECTION_LOWBAND[]  = "QI";
    constexpr char RX_FE_CONNECTION_HIGHBAND[] = "IQ";
    constexpr char TX_FE_CONNECTION_LOWBAND[]  = "QI";
    constexpr char TX_FE_CONNECTION_HIGHBAND[] = "IQ";

    constexpr int DEFAULT_IDENTIFY_DURATION = 5; // seconds

    const fs_path TX_FE_PATH = fs_path("tx_frontends") / 0 / "tune_args";
    const fs_path RX_FE_PATH = fs_path("rx_frontends") / 0 / "tune_args";

    device_addr_t _get_tune_args(uhd::property_tree::sptr tree, std::string _radio_slot, uhd::direction_t dir)
    {
        const auto subtree = tree->subtree(fs_path("dboards") / _radio_slot);
        const auto tune_arg_path = (dir == RX_DIRECTION) ? RX_FE_PATH : TX_FE_PATH;
        return subtree->access<device_addr_t>(tune_arg_path).get();
    }
}


/******************************************************************************
 * Structors
 *****************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(rhodium_radio_ctrl)
{
    UHD_LOG_TRACE(unique_id(), "Entering rhodium_radio_ctrl_impl ctor...");
    const char radio_slot_name[] = {'A', 'B'};
    _radio_slot = radio_slot_name[get_block_id().get_block_count()];
    _rpc_prefix =
        (_radio_slot == "A") ? "db_0_" : "db_1_";
    UHD_LOG_TRACE(unique_id(), "Radio slot: " << _radio_slot);
}

rhodium_radio_ctrl_impl::~rhodium_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "rhodium_radio_ctrl_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
double rhodium_radio_ctrl_impl::set_rate(double /* rate */)
{
    // TODO: implement
    // TODO: set_rate may also need to update the LO since master clock rate
    // changes also change the lowband LO frequency.  (run set_frequency)
    UHD_LOG_WARNING(unique_id(), "set_rate() called but not implemented");
    return 0.0;
}

void rhodium_radio_ctrl_impl::set_tx_antenna(
        const std::string &ant,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_tx_antenna(ant=" << ant << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    if (!uhd::has(RHODIUM_TX_ANTENNAS, ant)) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid TX antenna value: %s")
            % unique_id()
            % ant
        ));
    }

    _update_tx_output_switches(ant);
    // _update_atr will set the cached antenna value, so no need to do
    // it here. See comments in _update_antenna for more info.
    _update_atr(ant, TX_DIRECTION);
}

void rhodium_radio_ctrl_impl::set_rx_antenna(
        const std::string &ant,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "Setting RX antenna to " << ant);
    UHD_ASSERT_THROW(chan == 0);

    if (!uhd::has(RHODIUM_RX_ANTENNAS, ant)) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid RX antenna value: %s")
            % unique_id()
            % ant
        ));
    }

    _update_rx_input_switches(ant);
    // _update_atr will set the cached antenna value, so no need to do
    // it here. See comments in _update_antenna for more info.
    _update_atr(ant, RX_DIRECTION);
}

void rhodium_radio_ctrl_impl::_set_tx_fe_connection(const std::string &conn)
{
    UHD_LOG_TRACE(unique_id(), "set_tx_fe_connection(conn=" << conn  << ")");

    if (conn != _tx_fe_connection)
    {
        _tx_fe_core->set_mux(conn);
        _tx_fe_connection = conn;
    }
}

void rhodium_radio_ctrl_impl::_set_rx_fe_connection(const std::string &conn)
{
    UHD_LOG_TRACE(unique_id(), "set_rx_fe_connection(conn=" << conn <<  ")");

    if (conn != _tx_fe_connection)
    {
        _rx_fe_core->set_fe_connection(conn);
        _rx_fe_connection = conn;
    }
}

std::string rhodium_radio_ctrl_impl::_get_tx_fe_connection() const
{
    UHD_LOG_TRACE(unique_id(), "get_tx_fe_connection()");

    return _tx_fe_connection;
}

std::string rhodium_radio_ctrl_impl::_get_rx_fe_connection() const
{
    UHD_LOG_TRACE(unique_id(), "get_rx_fe_connection()");

    return _rx_fe_connection;
}

double rhodium_radio_ctrl_impl::set_tx_frequency(
        const double freq,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    double coerced_target_freq = uhd::clip(freq, RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ);

    if (freq != coerced_target_freq) {
        UHD_LOG_DEBUG(unique_id(), "Requested frequency is outside supported range. Coercing to " << coerced_target_freq);
    }

    const bool is_highband = !_is_tx_lowband(coerced_target_freq);

    const double target_lo_freq = is_highband ?
        coerced_target_freq : _get_lowband_lo_freq() - coerced_target_freq;
    const double actual_lo_freq =
        set_tx_lo_freq(target_lo_freq, RHODIUM_LO1, chan);
    const double coerced_freq = is_highband ?
        actual_lo_freq : _get_lowband_lo_freq() - actual_lo_freq;
    const auto conn = is_highband ?
        TX_FE_CONNECTION_HIGHBAND : TX_FE_CONNECTION_LOWBAND;

    _set_tx_fe_connection(conn);
    set_tx_gain(get_tx_gain(chan), 0);
    radio_ctrl_impl::set_tx_frequency(coerced_freq, chan);
    _update_tx_freq_switches(coerced_freq);
    // if TX lowband/highband changed and antenna is TX/RX,
    // the ATR needs to be updated
    _update_atr(get_tx_antenna(0), TX_DIRECTION);

    return coerced_freq;
}

double rhodium_radio_ctrl_impl::set_rx_frequency(
        const double freq,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    double coerced_target_freq = uhd::clip(freq, RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ);

    if (freq != coerced_target_freq) {
        UHD_LOG_DEBUG(unique_id(), "Requested frequency is outside supported range. Coercing to " << coerced_target_freq);
    }

    const bool is_highband = !_is_rx_lowband(coerced_target_freq);

    const double target_lo_freq = is_highband ?
        coerced_target_freq : _get_lowband_lo_freq() - coerced_target_freq;
    const double actual_lo_freq =
            set_rx_lo_freq(target_lo_freq, RHODIUM_LO1, chan);
    const double coerced_freq = is_highband ?
        actual_lo_freq : _get_lowband_lo_freq() - actual_lo_freq;
    const auto conn = is_highband ?
        RX_FE_CONNECTION_HIGHBAND : RX_FE_CONNECTION_LOWBAND;

    _set_rx_fe_connection(conn);
    set_rx_gain(get_rx_gain(chan), 0);
    radio_ctrl_impl::set_rx_frequency(coerced_freq, chan);
    _update_rx_freq_switches(coerced_freq);

    return coerced_freq;
}

double rhodium_radio_ctrl_impl::set_rx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_rx_bandwidth(bandwidth=" << bandwidth << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    return get_rx_bandwidth(chan);
}

double rhodium_radio_ctrl_impl::set_tx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_tx_bandwidth(bandwidth=" << bandwidth << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    return get_tx_bandwidth(chan);
}

double rhodium_radio_ctrl_impl::set_tx_gain(
        const double gain,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    auto freq = this->get_tx_frequency(chan);
    auto index = _get_gain_range(TX_DIRECTION).clip(gain);

    auto old_band = _is_tx_lowband(_tx_frequency_at_last_gain_write) ?
        rhodium_cpld_ctrl::gain_band_t::LOW :
        rhodium_cpld_ctrl::gain_band_t::HIGH;
    auto new_band = _is_tx_lowband(freq) ?
        rhodium_cpld_ctrl::gain_band_t::LOW :
        rhodium_cpld_ctrl::gain_band_t::HIGH;

    // The CPLD requires a rewrite of the gain control command on a change of lowband or highband
    if (get_tx_gain(chan) != index or old_band != new_band) {
        UHD_LOG_TRACE(unique_id(), "Writing new TX gain index: " << index);
        _cpld->set_gain_index(index, new_band, TX_DIRECTION);
        _tx_frequency_at_last_gain_write = freq;
        radio_ctrl_impl::set_tx_gain(index, chan);
    } else {
        UHD_LOG_TRACE(unique_id(), "No change in index or band, skipped writing TX gain index: " << index);
    }

    return index;
}

double rhodium_radio_ctrl_impl::set_rx_gain(
        const double gain,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    auto freq = this->get_rx_frequency(chan);
    auto index = _get_gain_range(RX_DIRECTION).clip(gain);

    auto old_band = _is_rx_lowband(_rx_frequency_at_last_gain_write) ?
        rhodium_cpld_ctrl::gain_band_t::LOW :
        rhodium_cpld_ctrl::gain_band_t::HIGH;
    auto new_band = _is_rx_lowband(freq) ?
        rhodium_cpld_ctrl::gain_band_t::LOW :
        rhodium_cpld_ctrl::gain_band_t::HIGH;

    // The CPLD requires a rewrite of the gain control command on a change of lowband or highband
    if (get_rx_gain(chan) != index or old_band != new_band) {
        UHD_LOG_TRACE(unique_id(), "Writing new RX gain index: " << index);
        _cpld->set_gain_index(index, new_band, RX_DIRECTION);
        _rx_frequency_at_last_gain_write = freq;
        radio_ctrl_impl::set_rx_gain(index, chan);
    } else {
        UHD_LOG_TRACE(unique_id(), "No change in index or band, skipped writing RX gain index: " << index);
    }

    return index;
}

void rhodium_radio_ctrl_impl::_update_atr(
    const std::string& ant,
    const direction_t dir
) {
    // This function updates sw10 based on the value of both antennas, so we
    // use a mutex to prevent other calls in this class instance from running
    // at the same time.
    std::lock_guard<std::mutex> lock(_ant_mutex);

    UHD_LOG_TRACE(unique_id(),
        "Updating ATRs for " << ((dir == RX_DIRECTION) ? "RX" : "TX") << " to " << ant);

    const auto rx_ant  = (dir == RX_DIRECTION) ? ant : get_rx_antenna(0);
    const auto tx_ant  = (dir == TX_DIRECTION) ? ant : get_tx_antenna(0);
    const auto sw10_tx = _is_tx_lowband(get_tx_frequency(0)) ?
        SW10_FROMTXLOWBAND : SW10_FROMTXHIGHBAND;


    const uint32_t atr_idle = SW10_ISOLATION;

    const uint32_t atr_rx = [rx_ant]{
        if (rx_ant == "TX/RX") {
            return SW10_TORX | LED_RX;
        } else if (rx_ant == "RX2") {
            return SW10_ISOLATION | LED_RX2;
        } else {
            return SW10_ISOLATION;
        }
    }();

    const uint32_t atr_tx = (tx_ant == "TX/RX") ?
        (sw10_tx | LED_TX) : SW10_ISOLATION;

    const uint32_t atr_dx = [tx_ant, rx_ant, sw10_tx] {
        uint32_t sw10_return;
        if (tx_ant == "TX/RX") {
            // if both channels are set to TX/RX, TX will override
            sw10_return = sw10_tx | LED_TX;
        } else if (rx_ant == "TX/RX") {
            sw10_return = SW10_TORX | LED_RX;
        } else {
            sw10_return = SW10_ISOLATION;
        }
        sw10_return |= (rx_ant == "RX2") ? LED_RX2 : 0;
        return sw10_return;
    }();

    _gpio->set_atr_reg(gpio_atr::ATR_REG_IDLE, atr_idle, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, atr_rx, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_TX_ONLY, atr_tx, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX, atr_dx, RHODIUM_GPIO_MASK);

    UHD_LOG_TRACE(unique_id(),
        str(boost::format("Wrote ATR registers i:0x%02X, r:0x%02X, t:0x%02X, d:0x%02X")
            % atr_idle % atr_rx % atr_tx % atr_dx));

    if (dir == RX_DIRECTION) {
        radio_ctrl_impl::set_rx_antenna(ant, 0);
    } else {
        radio_ctrl_impl::set_tx_antenna(ant, 0);
    }
}

uhd::gain_range_t rhodium_radio_ctrl_impl::_get_gain_range(direction_t dir)
{
    if (dir == RX_DIRECTION) {
        return gain_range_t(RX_MIN_GAIN, RX_MAX_GAIN, RX_GAIN_STEP);
    } else if (dir == TX_DIRECTION) {
        return gain_range_t(TX_MIN_GAIN, TX_MAX_GAIN, TX_GAIN_STEP);
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

bool rhodium_radio_ctrl_impl::_get_spur_dodging_enabled(uhd::direction_t dir) const
{
    UHD_ASSERT_THROW(_tree->exists(get_arg_path(SPUR_DODGING_ARG_NAME) / "value"));
    auto block_value = _tree->access<std::string>(get_arg_path(SPUR_DODGING_ARG_NAME) / "value").get();
    auto dict = _get_tune_args(_tree, _radio_slot, dir);

    // get the current tune_arg for spur_dodging
    // if the tune_arg doesn't exist, use the radio block argument instead
    std::string spur_dodging_arg = dict.cast<std::string>(
        SPUR_DODGING_ARG_NAME,
        block_value);

    if (spur_dodging_arg == "enabled")
    {
        UHD_LOG_TRACE(unique_id(), "_get_spur_dodging_enabled returning enabled");
        return true;
    }
    else if (spur_dodging_arg == "disabled") {
        UHD_LOG_TRACE(unique_id(), "_get_spur_dodging_enabled returning disabled");
        return false;
    }
    else {
        throw uhd::value_error(
            str(boost::format("Invalid spur_dodging argument: %s Valid options are [enabled, disabled]")
                % spur_dodging_arg));
    }
}

double rhodium_radio_ctrl_impl::_get_spur_dodging_threshold(uhd::direction_t dir) const
{
    UHD_ASSERT_THROW(_tree->exists(get_arg_path(SPUR_DODGING_THRESHOLD_ARG_NAME) / "value"));
    auto block_value = _tree->access<double>(get_arg_path(SPUR_DODGING_THRESHOLD_ARG_NAME) / "value").get();
    auto dict = _get_tune_args(_tree, _radio_slot, dir);

    // get the current tune_arg for spur_dodging_threshold
    // if the tune_arg doesn't exist, use the radio block argument instead
    auto threshold = dict.cast<double>(SPUR_DODGING_THRESHOLD_ARG_NAME, block_value);

    UHD_LOG_TRACE(unique_id(), "_get_spur_dodging_threshold returning " << threshold);

    return threshold;
}

size_t rhodium_radio_ctrl_impl::get_chan_from_dboard_fe(
    const std::string &fe, const direction_t /* dir */
) {
    UHD_ASSERT_THROW(boost::lexical_cast<size_t>(fe) == 0);
    return 0;
}

std::string rhodium_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan,
    const direction_t /* dir */
) {
    UHD_ASSERT_THROW(chan == 0);
    return "0";
}

void rhodium_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
) {
    _rpcc = rpcc;
    _block_args = block_args;

    // Get and verify the MCR before _init_peripherals, which will use this value
    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    _master_clock_rate = _rpcc->request_with_token<double>(_rpc_prefix + "get_master_clock_rate");
    if (block_args.cast<double>("master_clock_rate", _master_clock_rate)
        != _master_clock_rate) {
        throw uhd::runtime_error(str(
            boost::format("Master clock rate mismatch. Device returns %f MHz, "
                "but should have been %f MHz.")
            % (_master_clock_rate / 1e6)
            % (block_args.cast<double>(
                "master_clock_rate", _master_clock_rate) / 1e6)
        ));
    }
    UHD_LOG_DEBUG(unique_id(),
        "Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    radio_ctrl_impl::set_rate(_master_clock_rate);

    UHD_LOG_TRACE(unique_id(), "Checking for existence of Rhodium DB in slot " << _radio_slot);
    const auto dboard_info = _rpcc->request<std::vector<std::map<std::string, std::string>>>("get_dboard_info");

    // There is a bug that if only one DB is plugged into slot B the vector
    // will only have 1 element but not be correlated to slot B at all.
    // For now, we assume a 1 element array means the DB is in slot A.
    if (dboard_info.size() <= get_block_id().get_block_count())
    {
        UHD_LOG_DEBUG(unique_id(), "No DB detected in slot " << _radio_slot);
        // Name and master clock rate are needed for RFNoC init, so set the
        // name now and let this function continue to set the MCR
        _tree->subtree(fs_path("dboards") / _radio_slot / "tx_frontends" / "0")
            ->create<std::string>("name").set("Unknown");
        _tree->subtree(fs_path("dboards") / _radio_slot / "rx_frontends" / "0")
            ->create<std::string>("name").set("Unknown");
    }
    else {
        UHD_LOG_DEBUG(unique_id(),
            "Rhodium DB detected in slot " <<
            _radio_slot <<
            ". Serial: " <<
            dboard_info.at(get_block_id().get_block_count()).at("serial"));
        _init_defaults();
        _init_peripherals();
        _init_prop_tree();
    }

    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = DEFAULT_IDENTIFY_DURATION;
        }
        // TODO: Update this when LED control is added
        //UHD_LOG_INFO(unique_id(),
        //    "Running LED identification process for " << identify_duration
        //    << " seconds.");
        //_identify_with_leds(identify_duration);
    }
}

bool rhodium_radio_ctrl_impl::get_lo_lock_status(
    const direction_t dir
) const
{
    return
        ((dir == RX_DIRECTION) or _tx_lo->get_lock_status()) and
        ((dir == TX_DIRECTION) or _rx_lo->get_lock_status());
}

UHD_RFNOC_BLOCK_REGISTER(rhodium_radio_ctrl, "RhodiumRadio");
