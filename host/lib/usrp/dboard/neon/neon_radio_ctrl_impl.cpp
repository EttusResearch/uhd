//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "neon_radio_ctrl_impl.hpp"
#include "neon_constants.hpp"
#include "neon_regs.hpp"
#include <uhd/exception.hpp>
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
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(neon_radio_ctrl)
{
    UHD_LOG_TRACE(unique_id(), "Entering neon_radio_ctrl_impl ctor...");
    const char radio_slot_name[1] = {'A'};
    _radio_slot                   = radio_slot_name[get_block_id().get_block_count()];
    UHD_LOG_TRACE(unique_id(), "Radio slot: " << _radio_slot);
    _rpc_prefix = "db_0_";

    _init_defaults();
    _init_peripherals();
    _init_prop_tree();
}

neon_radio_ctrl_impl::~neon_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "neon_radio_ctrl_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/

bool neon_radio_ctrl_impl::check_radio_config()
{
    // mapping of frontend to radio perif index
    static const size_t FE0 = 0;
    static const size_t FE1 = 1;
    const size_t num_rx =
        _is_streamer_active(RX_DIRECTION, FE0) + _is_streamer_active(RX_DIRECTION, FE1);
    const size_t num_tx =
        _is_streamer_active(TX_DIRECTION, FE0) + _is_streamer_active(TX_DIRECTION, FE1);

    // setup the active chains in the codec
    if ((num_rx + num_tx) == 0) {
        // Ensure at least one RX chain is enabled so AD9361 outputs a sample clock
        this->set_streaming_mode(false, false, true, false);
    } else {
        this->set_streaming_mode(_is_streamer_active(TX_DIRECTION, FE0),
            _is_streamer_active(TX_DIRECTION, FE1),
            _is_streamer_active(RX_DIRECTION, FE0),
            _is_streamer_active(RX_DIRECTION, FE1));
    }
    return true;
}

void neon_radio_ctrl_impl::set_streaming_mode(
    const bool tx1, const bool tx2, const bool rx1, const bool rx2)
{
    UHD_LOG_TRACE(unique_id(), "Setting up streaming ...")
    const size_t num_rx = rx1 + rx2;
    const size_t num_tx = tx1 + tx2;

    // setup the active chains in the codec
    _ad9361->set_active_chains(tx1, tx2, rx1, rx2);

    const std::string TIMING_MODE_2R2T = "2R2T";
    const std::string TIMING_MODE_1R1T = "1R1T";
    const std::string MIMO             = "MIMO"; // 2R2T
    const std::string SISO_TX1         = "SISO_TX1"; // 1R1T
    const std::string SISO_TX0         = "SISO_TX0"; // 1R1T
    // setup 1R1T/2R2T mode in catalina and fpga
    // The Catalina interface in the fpga needs to know which TX channel to use for
    // the data on the LVDS lines.
    if ((num_rx == 2) or (num_tx == 2)) {
        // AD9361 is in 2R2T mode
        _ad9361->set_timing_mode(TIMING_MODE_2R2T);
        this->set_channel_mode(MIMO);
    } else {
        // AD9361 is in 1R1T mode
        _ad9361->set_timing_mode(TIMING_MODE_1R1T);

        // Set to SIS0_TX1 if we're using the second TX antenna, otherwise
        // default to SISO_TX0
        this->set_channel_mode(tx2 ? SISO_TX1 : SISO_TX0);
    }
}

void neon_radio_ctrl_impl::set_channel_mode(const std::string& channel_mode)
{
    // MIMO for 2R2T mode for 2 channels
    // SISO_TX1 for 1R1T mode for 1 channel - TX1
    // SISO_TX0 for 1R1T mode for 1 channel - TX0

    _rpcc->request_with_token<void>("set_channel_mode", channel_mode);
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
void neon_radio_ctrl_impl::loopback_self_test(
    std::function<void(uint32_t)> poker_functor, std::function<uint64_t()> peeker_functor)
{
    // Save current rate before running this test
    const double current_rate = this->get_rate();
    // Set 2R2T mode, stream on all channels
    this->set_streaming_mode(true, true, true, true);
    // Set maximum rate for 2R2T mode
    this->set_rate(30.72e6);
    // Put AD936x in loopback mode
    _ad9361->data_port_loopback(true);
    UHD_LOG_INFO(unique_id(), "Performing CODEC loopback test... ");
    size_t hash                     = size_t(time(NULL));
    constexpr size_t loopback_count = 100;

    // Allow some time for AD936x to enter loopback mode.
    // There is no clear statement in the documentation of how long it takes,
    // but UG-570 does say to "allow six ADC_CLK/64 clock cycles of flush time"
    // when leaving the TX or RX states.  That works out to ~75us at the
    // minimum clock rate of 5 MHz, which lines up with test results.
    // Sleeping 1ms is far more than enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    for (size_t i = 0; i < loopback_count; i++) {
        // Create test word
        boost::hash_combine(hash, i);
        const uint32_t word32 = uint32_t(hash) & 0xfff0fff0;
        // const uint32_t word32 = 0xCA00C100;
        // Write test word to codec_idle idle register (on TX side)
        poker_functor(word32);

        // Read back values - TX is lower 32-bits and RX is upper 32-bits
        const uint64_t rb_word64 = peeker_functor();
        const uint32_t rb_tx     = uint32_t(rb_word64 >> 32);
        const uint32_t rb_rx     = uint32_t(rb_word64 & 0xffffffff);

        // Compare TX and RX values to test word
        bool test_fail = word32 != rb_tx or word32 != rb_rx;
        if (test_fail) {
            UHD_LOG_WARNING(unique_id(),
                "CODEC loopback test failed! "
                    << boost::format("Expected: 0x%08X Received (TX/RX): 0x%08X/0x%08X")
                           % word32 % rb_tx % rb_rx);
            throw uhd::runtime_error("CODEC loopback test failed.");
        }
    }
    UHD_LOG_INFO(unique_id(), "CODEC loopback test passed");

    // Zero out the idle data.
    poker_functor(0);

    // Take AD936x out of loopback mode
    _ad9361->data_port_loopback(false);
    this->set_streaming_mode(true, false, true, false);
    // Switch back to current rate
    this->set_rate(current_rate);
}

double neon_radio_ctrl_impl::set_rate(const double rate)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_DEBUG(unique_id(), "Asking for clock rate " << rate / 1e6 << " MHz\n");
    double actual_tick_rate = _ad9361->set_clock_rate(rate);
    UHD_LOG_DEBUG(
        unique_id(), "Actual clock rate " << actual_tick_rate / 1e6 << " MHz\n");

    radio_ctrl_impl::set_rate(rate);
    return rate;
}

void neon_radio_ctrl_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid TX antenna value: %s")
                % unique_id() % ant));
    }
    radio_ctrl_impl::set_tx_antenna(ant, chan);
    // We can't actually set the TX antenna, so let's stop here.
}

void neon_radio_ctrl_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    UHD_ASSERT_THROW(chan <= NEON_NUM_CHANS);
    if (std::find(NEON_RX_ANTENNAS.begin(), NEON_RX_ANTENNAS.end(), ant)
        == NEON_RX_ANTENNAS.end()) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid RX antenna value: %s")
                % unique_id() % ant));
    }
    UHD_LOG_TRACE(unique_id(), "Setting RX antenna to " << ant << " for chan " << chan);

    radio_ctrl_impl::set_rx_antenna(ant, chan);
    _set_atr_bits(chan);
}

double neon_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    UHD_LOG_TRACE(unique_id(), "set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_TX_MIN_FREQ, AD9361_TX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(TX_DIRECTION, chan), clipped_freq);
    radio_ctrl_impl::set_tx_frequency(coerced_freq, chan);
    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

double neon_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    UHD_LOG_TRACE(unique_id(), "set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    std::lock_guard<std::mutex> l(_set_lock);

    double clipped_freq = uhd::clip(freq, AD9361_RX_MIN_FREQ, AD9361_RX_MAX_FREQ);

    double coerced_freq =
        _ad9361->tune(get_which_ad9361_chain(RX_DIRECTION, chan), clipped_freq);
    radio_ctrl_impl::set_rx_frequency(coerced_freq, chan);
    // Front-end switching
    _set_atr_bits(chan);

    return coerced_freq;
}

double neon_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    double clipped_bw =
        _ad9361->set_bw_filter(get_which_ad9361_chain(RX_DIRECTION, chan), bandwidth);
    return radio_ctrl_impl::set_rx_bandwidth(clipped_bw, chan);
}

double neon_radio_ctrl_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    double clipped_bw =
        _ad9361->set_bw_filter(get_which_ad9361_chain(TX_DIRECTION, chan), bandwidth);
    return radio_ctrl_impl::set_tx_bandwidth(clipped_bw, chan);
}

double neon_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_TX_GAIN, AD9361_MAX_TX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(TX_DIRECTION, chan), clip_gain);
    radio_ctrl_impl::set_tx_gain(clip_gain, chan);
    return clip_gain;
}

double neon_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = uhd::clip(gain, AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN);
    _ad9361->set_gain(get_which_ad9361_chain(RX_DIRECTION, chan), clip_gain);
    radio_ctrl_impl::set_rx_gain(clip_gain, chan);
    return clip_gain;
}

size_t neon_radio_ctrl_impl::get_chan_from_dboard_fe(
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

std::string neon_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan, const direction_t /* dir */
)
{
    return std::to_string(chan);
}

void neon_radio_ctrl_impl::set_rpc_client(
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
        for (size_t chan_idx = 0; chan_idx < NEON_NUM_CHANS; chan_idx++) {
            _init_mpm_sensors(dir, chan_idx);
        }
    }
}

bool neon_radio_ctrl_impl::get_lo_lock_status(const direction_t dir)
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

void neon_radio_ctrl_impl::_set_atr_bits(const size_t chan)
{
    const auto rx_freq       = radio_ctrl_impl::get_rx_frequency(chan);
    const auto tx_freq       = radio_ctrl_impl::get_tx_frequency(chan);
    const auto rx_ant        = radio_ctrl_impl::get_rx_antenna(chan);
    const uint32_t rx_regs   = _get_rx_switches(chan, rx_freq, rx_ant);
    const uint32_t tx_regs   = _get_tx_switches(chan, tx_freq);
    const uint32_t idle_regs = TX_AMP_OFF << TX_AMP_SHIFT | TRX1_SW_TX_HB << TRX_SW_SHIFT
                               | TX_SW2_LB_80 << TX_SW2_SHIFT
                               | TX_SW1_LB_80 << TX_SW1_SHIFT | RX_SW3_OFF << RX_SW3_SHIFT
                               | RX_SW2_OFF << RX_SW2_SHIFT | RX_SW1_OFF << RX_SW1_SHIFT;

    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, idle_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_RX_ONLY, rx_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_TX_ONLY, tx_regs);
    _db_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX, rx_regs | tx_regs);

    // The LED signal names are reversed, but are consistent with the schematic
    const int idle_led = 0;
    const bool is_txrx = rx_ant == "TX/RX";
    const int rx_led   = 1 << TRX_LED_GRN_SHIFT;
    const int tx_led   = 1 << TX_LED_RED_SHIFT;
    const int txrx_led = 1 << RX_LED_GRN_SHIFT;

    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, idle_led);
    _leds_gpio[chan]->set_atr_reg(
        usrp::gpio_atr::ATR_REG_RX_ONLY, is_txrx ? txrx_led : rx_led);
    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_TX_ONLY, tx_led);
    _leds_gpio[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX, rx_led | tx_led);
}

void neon_radio_ctrl_impl::_identify_with_leds(const int identify_duration)
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

uint32_t neon_radio_ctrl_impl::_get_tx_switches(const size_t chan, const double freq)
{
    UHD_LOG_TRACE(
        unique_id(), "Update all TX freq related switches. f=" << freq << " Hz, ");
    auto tx_sw1 = TX_SW1_LB_160;
    auto tx_sw2 = TX_SW2_LB_160;
    auto trx_sw = (chan == 0) ? TRX1_SW_TX_LB : TRX2_SW_TX_LB;
    auto tx_amp = TX_AMP_LF_ON;

    const auto band = _map_freq_to_tx_band(freq);
    switch (band) {
        case tx_band::LB_80:
            tx_sw1 = TX_SW1_LB_80;
            tx_sw2 = TX_SW2_LB_80;
            break;
        case tx_band::LB_160:
            tx_sw1 = TX_SW1_LB_160;
            tx_sw2 = TX_SW2_LB_160;
            break;
        case tx_band::LB_225:
            tx_sw1 = TX_SW1_LB_225;
            tx_sw2 = TX_SW2_LB_225;
            break;
        case tx_band::LB_400:
            tx_sw1 = TX_SW1_LB_400;
            tx_sw2 = TX_SW2_LB_400;
            break;
        case tx_band::LB_575:
            tx_sw1 = TX_SW1_LB_575;
            tx_sw2 = TX_SW2_LB_575;
            break;
        case tx_band::LB_1000:
            tx_sw1 = TX_SW1_LB_1000;
            tx_sw2 = TX_SW2_LB_1000;
            break;
        case tx_band::LB_1700:
            tx_sw1 = TX_SW1_LB_1700;
            tx_sw2 = TX_SW2_LB_1700;
            break;
        case tx_band::LB_2750:
            tx_sw1 = TX_SW1_LB_2750;
            tx_sw2 = TX_SW2_LB_2750;
            break;
        case tx_band::HB:
            trx_sw = (chan == 0) ? TRX1_SW_TX_HB : TRX2_SW_TX_HB;
            tx_amp = TX_AMP_HF_ON;
            break;
        case tx_band::INVALID_BAND:
            UHD_LOG_ERROR(unique_id(), "Cannot map TX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }

    auto tx_regs = tx_amp << TX_AMP_SHIFT | trx_sw << TRX_SW_SHIFT
                   | tx_sw2 << TX_SW2_SHIFT | tx_sw1 << TX_SW1_SHIFT;
    return tx_regs;
}

uint32_t neon_radio_ctrl_impl::_get_rx_switches(
    const size_t chan, const double freq, const std::string& ant)
{
    UHD_LOG_TRACE(
        unique_id(), "Update all RX freq related switches. f=" << freq << " Hz, ");
    // Default to OFF
    auto rx_sw1 = RX_SW1_OFF;
    auto rx_sw2 = RX_SW2_OFF;
    auto rx_sw3 = RX_SW3_OFF;
    auto trx_sw = (chan == 0) ? TRX1_SW_RX : TRX2_SW_RX;
    if (ant == "TX/RX") {
        rx_sw3 = RX_SW3_HBRX_LBTRX;
        trx_sw = (chan == 0) ? TRX1_SW_RX : TRX2_SW_RX;
    } else if (ant == "RX2") {
        rx_sw3 = RX_SW3_HBTRX_LBRX;
        // Set TRX switch to TX when receiving on RX2
        trx_sw = TRX1_SW_TX_HB;
    }

    const auto band = _map_freq_to_rx_band(freq);
    switch (band) {
        case rx_band::LB_B2:
            rx_sw1 = RX_SW1_LB_B2;
            rx_sw2 = RX_SW2_LB_B2;
            break;
        case rx_band::LB_B3:
            rx_sw1 = RX_SW1_LB_B3;
            rx_sw2 = RX_SW2_LB_B3;
            break;
        case rx_band::LB_B4:
            rx_sw1 = RX_SW1_LB_B4;
            rx_sw2 = RX_SW2_LB_B4;
            break;
        case rx_band::LB_B5:
            rx_sw1 = RX_SW1_LB_B5;
            rx_sw2 = RX_SW2_LB_B5;
            break;
        case rx_band::LB_B6:
            rx_sw1 = RX_SW1_LB_B6;
            rx_sw2 = RX_SW2_LB_B6;
            break;
        case rx_band::LB_B7:
            rx_sw1 = RX_SW1_LB_B7;
            rx_sw2 = RX_SW2_LB_B7;
            break;
        case rx_band::HB:
            rx_sw1 = RX_SW1_OFF;
            rx_sw2 = RX_SW2_OFF;
            if (ant == "TX/RX") {
                rx_sw3 = RX_SW3_HBTRX_LBRX;
            } else if (ant == "RX2") {
                rx_sw3 = RX_SW3_HBRX_LBTRX;
            }
            break;
        case rx_band::INVALID_BAND:
            UHD_LOG_ERROR(unique_id(), "Cannot map RX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }

    auto rx_regs = trx_sw << TRX_SW_SHIFT | rx_sw3 << RX_SW3_SHIFT
                   | rx_sw2 << RX_SW2_SHIFT | rx_sw1 << RX_SW1_SHIFT;
    return rx_regs;
}

UHD_RFNOC_BLOCK_REGISTER(neon_radio_ctrl, "NeonRadio");
