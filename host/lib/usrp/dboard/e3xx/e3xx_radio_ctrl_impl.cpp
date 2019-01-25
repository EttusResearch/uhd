//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_radio_ctrl_impl.hpp"
#include "e3xx_constants.hpp"
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <cmath>
#include <cstdlib>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

/******************************************************************************
 * Structors
 *****************************************************************************/
e3xx_radio_ctrl_impl::e3xx_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "Entering e3xx_radio_ctrl_impl ctor...");
    const char radio_slot_name[1] = {'A'};
    _radio_slot                   = radio_slot_name[get_block_id().get_block_count()];
    UHD_LOG_TRACE(unique_id(), "Radio slot: " << _radio_slot);
    _rpc_prefix = "db_0_";

    _init_defaults();
    _init_peripherals();
    _init_prop_tree();
}

e3xx_radio_ctrl_impl::~e3xx_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "e3xx_radio_ctrl_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/

void e3xx_radio_ctrl_impl::set_streaming_mode(
    const bool tx1, const bool tx2, const bool rx1, const bool rx2)
{
    UHD_LOG_TRACE(unique_id(), "Setting up streaming ...")
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

void e3xx_radio_ctrl_impl::set_channel_mode(const std::string& channel_mode)
{
    // MIMO for 2R2T mode for 2 channels
    // SISO_TX1 for 1R1T mode for 1 channel - TX1
    // SISO_TX0 for 1R1T mode for 1 channel - TX0

    _rpcc->request_with_token<void>("set_channel_mode", channel_mode);
}

double e3xx_radio_ctrl_impl::set_rate(const double rate)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_DEBUG(unique_id(), "Asking for clock rate " << rate / 1e6 << " MHz\n");
    double actual_tick_rate = _ad9361->set_clock_rate(rate);
    UHD_LOG_DEBUG(
        unique_id(), "Actual clock rate " << actual_tick_rate / 1e6 << " MHz\n");

    radio_ctrl_impl::set_rate(rate);
    return rate;
}

void e3xx_radio_ctrl_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid TX antenna value: %s")
                % unique_id() % ant));
    }
    radio_ctrl_impl::set_tx_antenna(ant, chan);
    // We can't actually set the TX antenna, so let's stop here.
}

void e3xx_radio_ctrl_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    UHD_ASSERT_THROW(chan <= E3XX_NUM_CHANS);
    if (std::find(E3XX_RX_ANTENNAS.begin(), E3XX_RX_ANTENNAS.end(), ant)
        == E3XX_RX_ANTENNAS.end()) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid RX antenna value: %s")
                % unique_id() % ant));
    }
    UHD_LOG_TRACE(unique_id(), "Setting RX antenna to " << ant << " for chan " << chan);

    radio_ctrl_impl::set_rx_antenna(ant, chan);
    _set_atr_bits(chan);
}

double e3xx_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    UHD_LOG_TRACE(unique_id(), "set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_TX_MIN_FREQ, AD9361_TX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), clipped_freq);
    radio_ctrl_impl::set_tx_frequency(coerced_freq, chan);
    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

double e3xx_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    UHD_LOG_TRACE(unique_id(), "set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_RX_MIN_FREQ, AD9361_RX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), clipped_freq);
    radio_ctrl_impl::set_rx_frequency(coerced_freq, chan);
    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

double e3xx_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    double clipped_bw =
        _ad9361->set_bw_filter(get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), bandwidth);
    return radio_ctrl_impl::set_rx_bandwidth(clipped_bw, chan);
}

double e3xx_radio_ctrl_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    double clipped_bw =
        _ad9361->set_bw_filter(get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), bandwidth);
    return radio_ctrl_impl::set_tx_bandwidth(clipped_bw, chan);
}

double e3xx_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_TX_GAIN, AD9361_MAX_TX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(TX_DIRECTION, chan, _fe_swap), clip_gain);
    radio_ctrl_impl::set_tx_gain(clip_gain, chan);
    return clip_gain;
}

double e3xx_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(RX_DIRECTION, chan, _fe_swap), clip_gain);
    radio_ctrl_impl::set_rx_gain(clip_gain, chan);
    return clip_gain;
}

size_t e3xx_radio_ctrl_impl::get_chan_from_dboard_fe(
    const std::string& fe, const direction_t /* dir */
)
{
    const size_t chan = boost::lexical_cast<size_t>(fe);
    if (chan > _get_num_radios() - 1) {
        UHD_LOG_WARNING(unique_id(),
            boost::format("Invalid channel determined from dboard frontend %s.") % fe);
    }
    return chan;
}

std::string e3xx_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan, const direction_t /* dir */
)
{
    return std::to_string(chan);
}

void e3xx_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc, const uhd::device_addr_t& block_args)
{
    _rpcc       = rpcc;
    _block_args = block_args;
    UHD_LOG_TRACE(unique_id(), "Instantiating AD9361 control object...");
    _ad9361 = make_rpc(_rpcc);

    UHD_LOG_TRACE(unique_id(), "Setting Catalina Defaults... ");
    // Initialize catalina
    this->_init_codec();

    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration          = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = 5;
        }
        UHD_LOG_INFO(unique_id(),
            "Running LED identification process for " << identify_duration
                                                      << " seconds.");
        _identify_with_leds(identify_duration);
    }
    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    _master_clock_rate =
        _rpcc->request_with_token<double>(_rpc_prefix + "get_master_clock_rate");
    if (block_args.cast<double>("master_clock_rate", _master_clock_rate)
        != _master_clock_rate) {
        throw uhd::runtime_error(str(
            boost::format("Master clock rate mismatch. Device returns %f MHz, "
                          "but should have been %f MHz.")
            % (_master_clock_rate / 1e6)
            % (block_args.cast<double>("master_clock_rate", _master_clock_rate) / 1e6)));
    }
    UHD_LOG_DEBUG(
        unique_id(), "Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    this->set_rate(_master_clock_rate);

    // Loopback test
    for (size_t chan = 0; chan < _get_num_radios(); chan++) {
        loopback_self_test(
            [this, chan](
                const uint32_t value) { this->sr_write(regs::CODEC_IDLE, value, chan); },
            [this, chan]() {
                return this->user_reg_read64(regs::RB_CODEC_READBACK, chan);
            });
    }

    const size_t db_idx = get_block_id().get_block_count();
    _tree->access<eeprom_map_t>(_root_path / "eeprom")
        .add_coerced_subscriber([this, db_idx](const eeprom_map_t& db_eeprom) {
            this->_rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
        })
        .set_publisher([this, db_idx]() {
            return this->_rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", db_idx);
        });

    // Init sensors
    for (const auto& dir : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        for (size_t chan_idx = 0; chan_idx < E3XX_NUM_CHANS; chan_idx++) {
            _init_mpm_sensors(dir, chan_idx);
        }
    }
}

bool e3xx_radio_ctrl_impl::get_lo_lock_status(const direction_t dir)
{
    if (not(bool(_rpcc))) {
        UHD_LOG_DEBUG(unique_id(), "Reported no LO lock due to lack of RPC connection.");
        return false;
    }

    const std::string trx = (dir == RX_DIRECTION) ? "rx" : "tx";
    bool lo_lock =
        _rpcc->request_with_token<bool>(_rpc_prefix + "get_ad9361_lo_lock", trx);
    UHD_LOG_TRACE(unique_id(),
        "AD9361 " << trx << " LO reports lock: " << (lo_lock ? "Yes" : "No"));

    return lo_lock;
}

void e3xx_radio_ctrl_impl::_set_atr_bits(const size_t chan)
{
    const auto rx_freq       = radio_ctrl_impl::get_rx_frequency(chan);
    const auto tx_freq       = radio_ctrl_impl::get_tx_frequency(chan);
    const auto rx_ant        = radio_ctrl_impl::get_rx_antenna(chan);
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

void e3xx_radio_ctrl_impl::_identify_with_leds(const int identify_duration)
{
    auto end_time =
        std::chrono::steady_clock::now() + std::chrono::seconds(identify_duration);
    bool led_state = true;
    while (std::chrono::steady_clock::now() < end_time) {
        // Add update_leds
        led_state = !led_state;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
