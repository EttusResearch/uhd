//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_radio_control_impl.hpp"
#include "e3xx_constants.hpp"
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <thread>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

/******************************************************************************
 * Structors
 *****************************************************************************/
e3xx_radio_control_impl::e3xx_radio_control_impl(make_args_ptr make_args)
    : radio_control_impl(std::move(make_args))
{
    RFNOC_LOG_TRACE("Entering e3xx_radio_control_impl ctor...");
    UHD_ASSERT_THROW(get_block_id().get_block_count() == 0);
    UHD_ASSERT_THROW(
        std::max(get_num_output_ports(), get_num_input_ports()) == E3XX_NUM_CHANS);
    UHD_ASSERT_THROW(get_mb_controller());
    _e3xx_mb_control = std::dynamic_pointer_cast<mpmd_mb_controller>(get_mb_controller());
    UHD_ASSERT_THROW(_e3xx_mb_control);
    _e3xx_timekeeper = std::dynamic_pointer_cast<mpmd_mb_controller::mpmd_timekeeper>(
        _e3xx_mb_control->get_timekeeper(0));
    UHD_ASSERT_THROW(_e3xx_timekeeper);
    _rpcc = _e3xx_mb_control->get_rpc_client();
    UHD_ASSERT_THROW(_rpcc);
    RFNOC_LOG_TRACE("Instantiating AD9361 control object...");
    _ad9361 = make_rpc(_rpcc);

    _init_defaults();
    _init_peripherals();
    _init_prop_tree();

    // Properties
    for (auto& samp_rate_prop : _samp_rate_in) {
        samp_rate_prop.set(_master_clock_rate);
    }
    for (auto& samp_rate_prop : _samp_rate_out) {
        samp_rate_prop.set(_master_clock_rate);
    }
}

e3xx_radio_control_impl::~e3xx_radio_control_impl()
{
    RFNOC_LOG_TRACE("e3xx_radio_control_impl::dtor() ");
}

void e3xx_radio_control_impl::deinit()
{
    _db_gpio.clear();
    _leds_gpio.clear();
    _fp_gpio.reset();
    _wb_ifaces.clear();
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
bool e3xx_radio_control_impl::check_topology(const std::vector<size_t>& connected_inputs,
    const std::vector<size_t>& connected_outputs)
{
    if (!node_t::check_topology(connected_inputs, connected_outputs)) {
        return false;
    }
    // Now we know that the connected ports are either 0 or 1

    // Check if we're running a 2x1 or 1x2 configuration -- the device does not
    // support this!
    if ((connected_outputs.size() == 1 && connected_inputs.size() == 2)
        || (connected_outputs.size() == 2 && connected_inputs.size() == 1)) {
        const std::string err_msg("Invalid channel configuration: This device does not "
                                  "support 1 TX x 2 RX or 2 TX x 1 RX configurations!");
        RFNOC_LOG_ERROR(err_msg);
        throw uhd::runtime_error(err_msg);
    }
    // mapping of frontend to radio perif index
    const size_t FE0 = _fe_swap ? 1 : 0;
    const size_t FE1 = _fe_swap ? 0 : 1;

    const bool tx_fe0_active = std::any_of(connected_inputs.begin(),
        connected_inputs.end(),
        [FE0](const size_t port) { return port == FE0; });
    const bool tx_fe1_active = std::any_of(connected_inputs.begin(),
        connected_inputs.end(),
        [FE1](const size_t port) { return port == FE1; });
    const bool rx_fe0_active = std::any_of(connected_outputs.begin(),
        connected_outputs.end(),
        [FE0](const size_t port) { return port == FE0; });
    const bool rx_fe1_active = std::any_of(connected_outputs.begin(),
        connected_outputs.end(),
        [FE1](const size_t port) { return port == FE1; });
    RFNOC_LOG_TRACE("TX FE0 Active: " << tx_fe0_active);
    RFNOC_LOG_TRACE("TX FE1 Active: " << tx_fe1_active);
    RFNOC_LOG_TRACE("RX FE0 Active: " << rx_fe0_active);
    RFNOC_LOG_TRACE("RX FE1 Active: " << rx_fe1_active);

    // setup the active chains in the codec
    if (connected_inputs.size() + connected_outputs.size() == 0) {
        // Ensure at least one RX chain is enabled so AD9361 outputs a sample clock
        this->set_streaming_mode(true, false, true, false);
    } else {
        this->set_streaming_mode(
            tx_fe0_active, tx_fe1_active, rx_fe0_active, rx_fe1_active);
    }
    return true;
}


void e3xx_radio_control_impl::set_streaming_mode(
    const bool tx1, const bool tx2, const bool rx1, const bool rx2)
{
    RFNOC_LOG_TRACE("Setting streaming mode...")
    const size_t num_rx = rx1 + rx2;
    const size_t num_tx = tx1 + tx2;

    // setup the active chains in the codec
    if ((num_rx + num_tx) == 0) {
        // Ensure at least one RX chain is enabled so AD9361 outputs a sample clock
        _ad9361->set_active_chains(true, false, true, false);
    } else {
        // setup the active chains in the codec
        _ad9361->set_active_chains(tx1, tx2, rx1, rx2);
    }

    // setup 1R1T/2R2T mode in catalina and fpga
    // The Catalina interface in the fpga needs to know which TX channel to use for
    // the data on the LVDS lines.
    if ((num_rx == 2) or (num_tx == 2)) {
        // AD9361 is in 1R1T mode
        _ad9361->set_timing_mode(this->get_default_timing_mode());
        this->set_channel_mode(MIMO);
    } else {
        // AD9361 is in 1R1T mode
        _ad9361->set_timing_mode(TIMING_MODE_1R1T);

        // Set to SIS0_TX1 if we're using the second TX antenna, otherwise
        // default to SISO_TX0
        this->set_channel_mode(tx2 ? SISO_TX1 : SISO_TX0);
    }
}

void e3xx_radio_control_impl::set_channel_mode(const std::string& channel_mode)
{
    // MIMO for 2R2T mode for 2 channels
    // SISO_TX1 for 1R1T mode for 1 channel - TX1
    // SISO_TX0 for 1R1T mode for 1 channel - TX0
    _rpcc->request_with_token<void>("set_channel_mode", channel_mode);
}

double e3xx_radio_control_impl::set_rate(const double rate)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_DEBUG("Asking for clock rate " << rate / 1e6 << " MHz\n");
    // On E3XX, tick rate and samp rate are always the same
    double actual_tick_rate = _ad9361->set_clock_rate(rate);
    RFNOC_LOG_DEBUG("Actual clock rate " << actual_tick_rate / 1e6 << " MHz\n");
    set_tick_rate(actual_tick_rate);
    radio_control_impl::set_rate(actual_tick_rate);
    _e3xx_timekeeper->update_tick_rate(rate);
    return rate;
}

uhd::meta_range_t e3xx_radio_control_impl::get_rate_range() const
{
    return _ad9361->get_clock_rate_range();
}

/******************************************************************************
 * RF API calls
 *****************************************************************************/
void e3xx_radio_control_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid TX antenna value: %s")
                % get_unique_id() % ant));
    }
    radio_control_impl::set_tx_antenna(ant, chan);
    // We can't actually set the TX antenna, so let's stop here.
}

void e3xx_radio_control_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    UHD_ASSERT_THROW(chan <= E3XX_NUM_CHANS);
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    if (std::find(E3XX_RX_ANTENNAS.begin(), E3XX_RX_ANTENNAS.end(), ant)
        == E3XX_RX_ANTENNAS.end()) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid RX antenna value: %s")
                % get_unique_id() % ant));
    }
    RFNOC_LOG_TRACE("Setting RX antenna to " << ant << " for chan " << chan);

    radio_control_impl::set_rx_antenna(ant, chan);
    _set_atr_bits(chan);
}

double e3xx_radio_control_impl::set_tx_frequency(const double freq, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_TX_MIN_FREQ, AD9361_TX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), clipped_freq);
    // The E3xx devices have one LO for TX, so if we change one channel's
    // frequency, we change the other, too
    for (size_t chan_idx = 0; chan_idx < E3XX_NUM_CHANS; ++chan_idx) {
        radio_control_impl::set_tx_frequency(coerced_freq, chan_idx);
    }

    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

double e3xx_radio_control_impl::set_rx_frequency(const double freq, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_RX_MIN_FREQ, AD9361_RX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), clipped_freq);
    // The E3xx devices have one LO for RX, so if we change one channel's
    // frequency, we change the other, too
    for (size_t chan_idx = 0; chan_idx < E3XX_NUM_CHANS; ++chan_idx) {
        radio_control_impl::set_rx_frequency(coerced_freq, chan_idx);
    }
    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

void e3xx_radio_control_impl::set_rx_agc(const bool enb, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_rx_agc(enb=" << enb << ", chan=" << chan << ")");
    const std::string rx_fe = get_which_ad9361_chain(RX_DIRECTION, chan);
    _ad9361->set_agc(rx_fe, enb);
}

double e3xx_radio_control_impl::set_rx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    double clipped_bw = _ad9361->set_bw_filter(
        get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), bandwidth);
    return radio_control_impl::set_rx_bandwidth(clipped_bw, chan);
}

double e3xx_radio_control_impl::set_tx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    double clipped_bw = _ad9361->set_bw_filter(
        get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), bandwidth);
    return radio_control_impl::set_tx_bandwidth(clipped_bw, chan);
}

double e3xx_radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_TX_GAIN, AD9361_MAX_TX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), clip_gain);
    radio_control_impl::set_tx_gain(clip_gain, chan);
    return clip_gain;
}

double e3xx_radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    UHD_ASSERT_THROW(chan < get_num_output_ports());
    RFNOC_LOG_TRACE("set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), clip_gain);
    radio_control_impl::set_rx_gain(clip_gain, chan);
    return clip_gain;
}

std::vector<std::string> e3xx_radio_control_impl::get_tx_antennas(const size_t) const
{
    return {E3XX_DEFAULT_TX_ANTENNA};
}

std::vector<std::string> e3xx_radio_control_impl::get_rx_antennas(const size_t) const
{
    return E3XX_RX_ANTENNAS;
}

freq_range_t e3xx_radio_control_impl::get_tx_frequency_range(const size_t) const
{
    return freq_range_t(AD9361_TX_MIN_FREQ, AD9361_TX_MAX_FREQ, 1.0);
}

freq_range_t e3xx_radio_control_impl::get_rx_frequency_range(const size_t) const
{
    return freq_range_t(AD9361_RX_MIN_FREQ, AD9361_RX_MAX_FREQ, 1.0);
}

uhd::gain_range_t e3xx_radio_control_impl::get_tx_gain_range(const size_t) const
{
    return meta_range_t(AD9361_MIN_TX_GAIN, AD9361_MAX_TX_GAIN, AD9361_TX_GAIN_STEP);
}

uhd::gain_range_t e3xx_radio_control_impl::get_rx_gain_range(const size_t) const
{
    return meta_range_t(AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN, AD9361_RX_GAIN_STEP);
}

meta_range_t e3xx_radio_control_impl::get_tx_bandwidth_range(size_t) const
{
    return meta_range_t(AD9361_TX_MIN_BANDWIDTH, AD9361_TX_MAX_BANDWIDTH);
}

meta_range_t e3xx_radio_control_impl::get_rx_bandwidth_range(size_t) const
{
    return meta_range_t(AD9361_RX_MIN_BANDWIDTH, AD9361_RX_MAX_BANDWIDTH);
}

/**************************************************************************
 * Calibration-Related API Calls
 *************************************************************************/
void e3xx_radio_control_impl::set_rx_dc_offset(const bool enb, size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_rx_dc_offset(enb=" << enb << ", chan=" << chan << ")");
    const std::string rx_fe = get_which_ad9361_chain(RX_DIRECTION, chan);
    _ad9361->set_dc_offset_auto(rx_fe, enb);
}

void e3xx_radio_control_impl::set_rx_iq_balance(const bool enb, size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_rx_iq_balance(enb=" << enb << ", chan=" << chan << ")");
    const std::string rx_fe = get_which_ad9361_chain(RX_DIRECTION, chan);
    _ad9361->set_iq_balance_auto(rx_fe, enb);
}

/**************************************************************************
 * GPIO Controls
 *************************************************************************/
void e3xx_radio_control_impl::set_gpio_attr(
    const std::string& bank, const std::string& attr, const uint32_t value)
{
    if (bank != get_gpio_banks().front()) {
        RFNOC_LOG_ERROR("Invalid GPIO bank: " << bank);
        throw uhd::key_error("Invalid GPIO bank!");
    }
    if (!gpio_atr::gpio_attr_rev_map.count(attr)) {
        RFNOC_LOG_ERROR("Invalid GPIO attr: " << attr);
        throw uhd::key_error("Invalid GPIO attr!");
    }

    const gpio_atr::gpio_attr_t gpio_attr = gpio_atr::gpio_attr_rev_map.at(attr);

    if (gpio_attr == gpio_atr::GPIO_READBACK) {
        RFNOC_LOG_WARNING("Cannot set READBACK attr.");
        return;
    }

    _fp_gpio->set_gpio_attr(gpio_attr, value);
}

uint32_t e3xx_radio_control_impl::get_gpio_attr(
    const std::string& bank, const std::string& attr)
{
    if (bank != get_gpio_banks().front()) {
        RFNOC_LOG_ERROR("Invalid GPIO bank: " << bank);
        throw uhd::key_error("Invalid GPIO bank!");
    }

    const gpio_atr::gpio_attr_t gpio_attr = gpio_atr::gpio_attr_rev_map.at(attr);
    return _fp_gpio->get_attr_reg(gpio_attr);
}

/******************************************************************************
 * Sensor API
 *****************************************************************************/
std::vector<std::string> e3xx_radio_control_impl::get_rx_sensor_names(const size_t) const
{
    return _rx_sensor_names;
}

uhd::sensor_value_t e3xx_radio_control_impl::get_rx_sensor(
    const std::string& sensor_name, const size_t chan)
{
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "RX", sensor_name, chan));
}

std::vector<std::string> e3xx_radio_control_impl::get_tx_sensor_names(const size_t) const
{
    return _tx_sensor_names;
}

uhd::sensor_value_t e3xx_radio_control_impl::get_tx_sensor(
    const std::string& sensor_name, const size_t chan)
{
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "TX", sensor_name, chan));
}

/*  loopback_self_test checks the integrity of the FPGA->AD936x->FPGA sample interface.
    The AD936x is put in loopback mode that sends the TX data unchanged to the RX side.
    A test value is written to the codec_idle register in the TX side of the radio.
    The readback register is then used to capture the values on the TX and RX sides
    simultaneously for comparison. It is a reasonably effective test for AC timing
    since I/Q Ch0/Ch1 alternate over the same wires. Note, however, that it uses
    whatever timing is configured at the time the test is called rather than select
    worst case conditions to stress the interface.
    Note: This currently only tests 2R2T mode
*/
void e3xx_radio_control_impl::loopback_self_test(const size_t chan)
{
    // Save current rate before running this test
    const double current_rate = this->get_rate();
    // Set 2R2T mode, stream on all channels
    this->set_streaming_mode(true, true, true, true);
    _ad9361->set_clock_rate(30.72e6);
    // Put AD936x in loopback mode
    _ad9361->data_port_loopback(true);
    RFNOC_LOG_INFO(
        "Performing CODEC loopback test on channel " << std::to_string(chan) << " ... ");
    size_t hash                     = size_t(time(NULL));
    constexpr size_t loopback_count = 100;
    constexpr size_t retries        = 3;
    // Allow some time for AD936x to enter loopback mode.
    // There is no clear statement in the documentation of how long it takes,
    // but UG-570 does say to "allow six ADC_CLK/64 clock cycles of flush time"
    // when leaving the TX or RX states.  That works out to ~75us at the
    // minimum clock rate of 5 MHz, which lines up with test results.
    // Sleeping 1ms is far more than enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    bool test_fail = true;
    while (test_fail) {
        size_t tries = 0;
        for (size_t i = 0; i < loopback_count; i++) {
            // Create test word
            boost::hash_combine(hash, i);
            const uint32_t word32 = uint32_t(hash) & 0xfff0fff0;
            // Write test word to codec_idle idle register (on TX side)
            regs().poke32(regmap::RADIO_BASE_ADDR + chan * regmap::REG_CHAN_OFFSET
                              + regmap::REG_TX_IDLE_VALUE,
                word32);

            // Read back values - TX is lower 32-bits and RX is upper 32-bits
            const uint32_t rb_tx =
                regs().peek32(regmap::RADIO_BASE_ADDR + chan * regmap::REG_CHAN_OFFSET
                              + regmap::REG_TX_IDLE_VALUE);
            const uint32_t rb_rx =
                regs().peek32(regmap::RADIO_BASE_ADDR + chan * regmap::REG_CHAN_OFFSET
                              + regmap::REG_RX_DATA);

            // Compare TX and RX values to test word
            test_fail = word32 != rb_tx or word32 != rb_rx;
            if (test_fail) {
                RFNOC_LOG_DEBUG(
                    "CODEC loopback test failure: "
                    << boost::format("Expected: 0x%08X Received (TX/RX): 0x%08X/0x%08X")
                           % word32 % rb_tx % rb_rx);
                if (tries < retries) {
                    // TODO: Investigate why loopback test sometimes fails
                    // Upon failure, setting the streaming mode again makes
                    // it work for some reason.
                    this->set_streaming_mode(true, true, true, true);

                    // Try again
                    RFNOC_LOG_DEBUG("Retrying CODEC loopback test for channel "
                                    << std::to_string(chan) << " ... ");
                    tries++;
                    break;
                } else {
                    RFNOC_LOG_ERROR("CODEC loopback test failed!");
                    throw uhd::runtime_error("CODEC loopback test failed.");
                }
            }
        }
    }
    RFNOC_LOG_INFO("CODEC loopback test passed");

    // Zero out the idle data.
    regs().poke32(regmap::RADIO_BASE_ADDR + chan * regmap::REG_CHAN_OFFSET
                      + regmap::REG_TX_IDLE_VALUE,
        0);

    // Take AD936x out of loopback mode
    _ad9361->data_port_loopback(false);
    this->set_streaming_mode(true, false, true, false);
    // Switch back to current rate
    _ad9361->set_clock_rate(current_rate);
}

void e3xx_radio_control_impl::_identify_with_leds(const int identify_duration)
{
    RFNOC_LOG_INFO(
        "Running LED identification process for " << identify_duration << " seconds.");
    auto end_time =
        std::chrono::steady_clock::now() + std::chrono::seconds(identify_duration);
    bool led_state = true;
    while (std::chrono::steady_clock::now() < end_time) {
        // Add update_leds
        led_state = !led_state;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void e3xx_radio_control_impl::_set_atr_bits(const size_t chan)
{
    const auto rx_freq       = radio_control_impl::get_rx_frequency(chan);
    const auto tx_freq       = radio_control_impl::get_tx_frequency(chan);
    const auto rx_ant        = radio_control_impl::get_rx_antenna(chan);
    const uint32_t rx_regs   = this->get_rx_switches(chan, rx_freq, rx_ant);
    const uint32_t tx_regs   = this->get_tx_switches(chan, tx_freq);
    const uint32_t idle_regs = this->get_idle_switches();

    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, idle_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_RX_ONLY, rx_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_TX_ONLY, tx_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX, rx_regs | tx_regs);

    // The LED signal names are reversed, but are consistent with the schematic
    const bool is_txrx = rx_ant == "TX/RX";
    const int idle_led = 0;
    const int rx_led   = this->get_rx_led();
    const int tx_led   = this->get_tx_led();
    const int txrx_led = this->get_txrx_led();

    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, idle_led);
    _leds_gpio[chan]->set_atr_reg(
        usrp::gpio_atr::ATR_REG_RX_ONLY, is_txrx ? txrx_led : rx_led);
    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_TX_ONLY, tx_led);
    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX, rx_led | tx_led);
}

void e3xx_radio_control_impl::set_db_eeprom(const eeprom_map_t& db_eeprom)
{
    _rpcc->notify_with_token("set_db_eeprom", 0, db_eeprom);
}

eeprom_map_t e3xx_radio_control_impl::get_db_eeprom()
{
    return _rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", 0);
}

/**************************************************************************
 * Filter API
 *************************************************************************/
std::vector<std::string> e3xx_radio_control_impl::get_rx_filter_names(const size_t) const
{
    return _rx_filter_names;
}

uhd::filter_info_base::sptr e3xx_radio_control_impl::get_rx_filter(
    const std::string& name, const size_t chan)
{
    return _ad9361->get_filter(
        get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), name);
}

void e3xx_radio_control_impl::set_rx_filter(
    const std::string& name, uhd::filter_info_base::sptr filter, const size_t chan)
{
    _ad9361->set_filter(
        get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), name, filter);
}

std::vector<std::string> e3xx_radio_control_impl::get_tx_filter_names(const size_t) const
{
    return _tx_filter_names;
}

uhd::filter_info_base::sptr e3xx_radio_control_impl::get_tx_filter(
    const std::string& name, const size_t chan)
{
    return _ad9361->get_filter(
        get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), name);
}

void e3xx_radio_control_impl::set_tx_filter(
    const std::string& name, uhd::filter_info_base::sptr filter, const size_t chan)
{
    _ad9361->set_filter(
        get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), name, filter);
}


/**************************************************************************
 * Radio Identification API Calls
 *************************************************************************/
size_t e3xx_radio_control_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t) const
{
    // A and B are available here for backward compat
    if (fe == "A" || fe == "0") {
        return 0;
    }
    if (fe == "B" || fe == "1") {
        return 1;
    }
    throw uhd::key_error(std::string("[E3xx] Invalid frontend: ") + fe);
}

std::string e3xx_radio_control_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t) const
{
    if (chan == 0) {
        return "0";
    }
    if (chan == 1) {
        return "1";
    }
    throw uhd::lookup_error(
        std::string("[E3xx] Invalid channel: ") + std::to_string(chan));
}

std::string e3xx_radio_control_impl::get_fe_name(
    const size_t, const uhd::direction_t) const
{
    return "E3xx";
}
