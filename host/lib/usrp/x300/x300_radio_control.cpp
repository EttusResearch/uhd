//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../dboard/db_basic_and_lf.hpp"
#include "x300_adc_ctrl.hpp"
#include "x300_dac_ctrl.hpp"
#include "x300_dboard_iface.hpp"
#include "x300_device_args.hpp"
#include "x300_mb_controller.hpp"
#include "x300_radio_mbc_iface.hpp"
#include "x300_regs.hpp"
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/rx_frontend_core_3000.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_frontend_core_200.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {

std::vector<uint8_t> str_to_bytes(std::string str)
{
    return std::vector<uint8_t>(str.cbegin(), str.cend());
}

std::string bytes_to_str(std::vector<uint8_t> str_b)
{
    return std::string(str_b.cbegin(), str_b.cend());
}

gain_fcns_t make_gain_fcns_from_subtree(property_tree::sptr subtree)
{
    gain_fcns_t gain_fcns;
    gain_fcns.get_range = [subtree]() {
        return subtree->access<meta_range_t>("range").get();
    };
    gain_fcns.get_value = [subtree]() { return subtree->access<double>("value").get(); };
    gain_fcns.set_value = [subtree](const double gain) {
        subtree->access<double>("value").set(gain);
    };
    return gain_fcns;
}

template <typename map_type>
size_t _get_chan_from_map(std::unordered_map<size_t, map_type> map, const std::string& fe)
{
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->second.db_fe_name == fe) {
            return it->first;
        }
    }
    throw uhd::lookup_error(
        str(boost::format("Invalid daughterboard frontend name: %s") % fe));
}

constexpr double DEFAULT_RATE  = 200e6;
constexpr char HW_GAIN_STAGE[] = "hw";

} // namespace

namespace x300_regs {

static constexpr uint32_t PERIPH_BASE       = 0x80000;
static constexpr uint32_t PERIPH_REG_OFFSET = 8;

// db_control registers
static constexpr uint32_t SR_MISC_OUTS = PERIPH_BASE + 160 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_SPI       = PERIPH_BASE + 168 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_LEDS      = PERIPH_BASE + 176 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_FP_GPIO   = PERIPH_BASE + 184 * PERIPH_REG_OFFSET;
static constexpr uint32_t SR_DB_GPIO   = PERIPH_BASE + 192 * PERIPH_REG_OFFSET;

// LED bit positions
// Green LED on TX/RX port (left SMA)
static constexpr int SR_LED_TXRX_RX = (1 << 0);
// Red LED on TX/RX port (left SMA)
static constexpr int SR_LED_TXRX_TX = (1 << 1);
// Green LED on RX2 port (right SMA)
static constexpr int SR_LED_RX2_RX = (1 << 2);

static constexpr uint32_t RB_MISC_IO = PERIPH_BASE + 16 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_SPI     = PERIPH_BASE + 17 * PERIPH_REG_OFFSET;
// static constexpr uint32_t RB_LEDS    = PERIPH_BASE + 18 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_DB_GPIO = PERIPH_BASE + 19 * PERIPH_REG_OFFSET;
static constexpr uint32_t RB_FP_GPIO = PERIPH_BASE + 20 * PERIPH_REG_OFFSET;


//! Delta between frontend offsets for channel 0 and 1
constexpr uint32_t SR_FE_CHAN_OFFSET = 16 * PERIPH_REG_OFFSET;
constexpr uint32_t SR_TX_FE_BASE     = PERIPH_BASE + 208 * PERIPH_REG_OFFSET;
constexpr uint32_t SR_RX_FE_BASE     = PERIPH_BASE + 224 * PERIPH_REG_OFFSET;

} // namespace x300_regs

class x300_radio_control_impl : public radio_control_impl,
                                public uhd::usrp::x300::x300_radio_mbc_iface
{
public:
    RFNOC_RADIO_CONSTRUCTOR(x300_radio_control)
    , _radio_type(get_block_id().get_block_count() == 0 ? PRIMARY : SECONDARY)
    {
        RFNOC_LOG_TRACE("Initializing x300_radio_control, slot "
                        << x300_radio_control_impl::get_slot_name());
        UHD_ASSERT_THROW(get_mb_controller());
        _x300_mb_control =
            std::dynamic_pointer_cast<x300_mb_controller>(get_mb_controller());
        UHD_ASSERT_THROW(_x300_mb_control);
        _x300_mb_control->register_radio(this);
        // MCR is locked for this session
        _master_clock_rate = _x300_mb_control->get_clock_ctrl()->get_master_clock_rate();
        UHD_ASSERT_THROW(get_tick_rate() == _master_clock_rate);
        radio_control_impl::set_rate(_master_clock_rate);

        ////////////////////////////////////////////////////////////////
        // Setup peripherals
        ////////////////////////////////////////////////////////////////
        // The X300 only requires a single timed_wb_iface, even for TwinRX
        _wb_iface = RFNOC_MAKE_WB_IFACE(0, 0);

        RFNOC_LOG_TRACE("Creating SPI interface...");
        _spi = spi_core_3000::make(
            [this](const uint32_t addr, const uint32_t data) {
                regs().poke32(addr, data, get_command_time(0));
            },
            [this](
                const uint32_t addr) { return regs().peek32(addr, get_command_time(0)); },
            x300_regs::SR_SPI,
            8,
            x300_regs::RB_SPI);
        // DAC/ADC
        RFNOC_LOG_TRACE("Running init_codec...");
        // Note: ADC calibration and DAC sync happen in x300_mb_controller
        _init_codecs();
        _x300_mb_control->register_reset_codec_cb([this]() { this->reset_codec(); });
        // FP-GPIO (the gpio_atr_3000 ctor will initialize default values)
        RFNOC_LOG_TRACE("Creating FP-GPIO interface...");
        _fp_gpio = gpio_atr::gpio_atr_3000::make(_wb_iface,
            gpio_atr::gpio_atr_offsets::make_default(x300_regs::SR_FP_GPIO,
                x300_regs::RB_FP_GPIO,
                x300_regs::PERIPH_REG_OFFSET));
        // Create the GPIO banks and attributes, and populate them with some default
        // values
        // TODO: Do we need this section? Since the _fp_gpio handles state now, we
        // don't need to stash values here. We only need this if we want to set
        // anything to a default value.
        for (const auto& attr : gpio_atr::gpio_attr_map) {
            // TODO: Default values?
            if (attr.first == usrp::gpio_atr::GPIO_SRC) {
                // Don't set the SRC
                // TODO: Remove from the map??
                continue;
            }
            set_gpio_attr("FP0", usrp::gpio_atr::gpio_attr_map.at(attr.first), 0);
        }
        // DB Initialization
        _init_db(); // This does not init the dboards themselves!

        // LEDs are technically valid for both RX and TX, but let's put them
        // here
        _leds = gpio_atr::gpio_atr_3000::make(_wb_iface,
            gpio_atr::gpio_atr_offsets::make_write_only(
                x300_regs::SR_LEDS, x300_regs::PERIPH_REG_OFFSET));
        _leds->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
        // Set LEDs to default setting: RX2 LED on RX, TX/RX red LED on TX. The
        // actual setting depends on the antenna choice and is handled in
        // _update_atr_leds().
        _leds->set_atr_reg(gpio_atr::ATR_REG_IDLE, 0);
        _leds->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, x300_regs::SR_LED_RX2_RX);
        _leds->set_atr_reg(gpio_atr::ATR_REG_TX_ONLY, x300_regs::SR_LED_TXRX_TX);
        // We choose to light both LEDs on full duplex, regardless of the
        // antenna selection, because the single multi-color LED on the TX/RX
        // side does not provide useful visual feedback by itself.
        _leds->set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX,
            x300_regs::SR_LED_RX2_RX | x300_regs::SR_LED_TXRX_TX);
        // We always want to initialize at least one frontend core for both TX and RX
        // RX periphs
        for (size_t i = 0; i < std::max<size_t>(get_num_output_ports(), 1); i++) {
            _rx_fe_map[i].core = rx_frontend_core_3000::make(_wb_iface,
                x300_regs::SR_RX_FE_BASE + i * x300_regs::SR_FE_CHAN_OFFSET,
                x300_regs::PERIPH_REG_OFFSET);
            _rx_fe_map[i].core->set_adc_rate(
                _x300_mb_control->get_clock_ctrl()->get_master_clock_rate());
            _rx_fe_map[i].core->set_dc_offset(
                rx_frontend_core_3000::DEFAULT_DC_OFFSET_VALUE);
            _rx_fe_map[i].core->set_dc_offset_auto(
                rx_frontend_core_3000::DEFAULT_DC_OFFSET_ENABLE);
            _rx_fe_map[i].core->populate_subtree(
                get_tree()->subtree(FE_PATH / "rx_fe_corrections" / i));
        }
        // TX Periphs
        for (size_t i = 0; i < std::max<size_t>(get_num_input_ports(), 1); i++) {
            _tx_fe_map[i].core = tx_frontend_core_200::make(_wb_iface,
                x300_regs::SR_TX_FE_BASE + i * x300_regs::SR_FE_CHAN_OFFSET,
                x300_regs::PERIPH_REG_OFFSET);
            _tx_fe_map[i].core->set_dc_offset(
                tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
            _tx_fe_map[i].core->set_iq_balance(
                tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
            _tx_fe_map[i].core->populate_subtree(
                get_tree()->subtree(FE_PATH / "tx_fe_corrections" / i));
        }

        // Dboards
        _init_dboards();

        // Properties
        for (auto& samp_rate_prop : _samp_rate_in) {
            set_property(
                samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
        }
        for (auto& samp_rate_prop : _samp_rate_out) {
            set_property(
                samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
        }
    } /* ctor */

    ~x300_radio_control_impl() override
    {
        // nop
    }

    /**************************************************************************
     * Radio API calls
     *************************************************************************/
    double set_rate(double rate) override
    {
        // On X3x0, tick rate can't actually be changed at runtime
        const double actual_rate = get_rate();
        if (not uhd::math::frequencies_are_equal(rate, actual_rate)) {
            RFNOC_LOG_WARNING("Requesting invalid sampling rate from device: "
                              << (rate / 1e6) << " MHz. Actual rate is: "
                              << (actual_rate / 1e6) << " MHz.");
        }
        return actual_rate;
    }

    void set_tx_antenna(const std::string& ant, const size_t chan) override
    {
        // Antenna changes may result in a change of streaming mode for LF/Basic dboards
        if (_basic_lf_tx) {
            if (not uhd::usrp::dboard::basic_and_lf::antenna_mode_to_conn.has_key(ant)) {
                throw uhd::lookup_error(
                    str(boost::format("Invalid antenna mode: %s") % ant));
            }
            const std::string connection =
                uhd::usrp::dboard::basic_and_lf::antenna_mode_to_conn[ant];
            _tx_fe_map[chan].core->set_mux(connection);
        }
        get_tree()
            ->access<std::string>(get_db_path("tx", chan) / "antenna" / "value")
            .set(ant);
    }

    std::string get_tx_antenna(const size_t chan) const override
    {
        return get_tree()
            ->access<std::string>(get_db_path("tx", chan) / "antenna" / "value")
            .get();
    }

    std::vector<std::string> get_tx_antennas(size_t chan) const override
    {
        return get_tree()
            ->access<std::vector<std::string>>(
                get_db_path("tx", chan) / "antenna" / "options")
            .get();
    }

    void set_rx_antenna(const std::string& ant, const size_t chan) override
    {
        // Antenna changes may result in a change of streaming mode for LF/Basic dboards
        if (_basic_lf_rx) {
            if (not uhd::usrp::dboard::basic_and_lf::antenna_mode_to_conn.has_key(ant)) {
                throw uhd::lookup_error(
                    str(boost::format("Invalid antenna mode: %s") % ant));
            }
            const std::string connection =
                uhd::usrp::dboard::basic_and_lf::antenna_mode_to_conn[ant];
            const double if_freq = 0.0;
            _rx_fe_map[chan].core->set_fe_connection(
                usrp::fe_connection_t(connection, if_freq));
        }
        get_tree()
            ->access<std::string>(get_db_path("rx", chan) / "antenna" / "value")
            .set(ant);
    }

    std::string get_rx_antenna(const size_t chan) const override
    {
        return get_tree()
            ->access<std::string>(get_db_path("rx", chan) / "antenna" / "value")
            .get();
    }

    std::vector<std::string> get_rx_antennas(size_t chan) const override
    {
        return get_tree()
            ->access<std::vector<std::string>>(
                get_db_path("rx", chan) / "antenna" / "options")
            .get();
    }

    double set_tx_frequency(const double freq, const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("tx", chan) / "freq" / "value")
            .set(freq)
            .get();
    }

    void set_tx_tune_args(const uhd::device_addr_t& tune_args, const size_t chan) override
    {
        if (get_tree()->exists(get_db_path("tx", chan) / "tune_args")) {
            get_tree()
                ->access<uhd::device_addr_t>(get_db_path("tx", chan) / "tune_args")
                .set(tune_args);
        }
    }

    double get_tx_frequency(const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("tx", chan) / "freq" / "value")
            .get();
    }

    double set_rx_frequency(const double freq, const size_t chan) override
    {
        RFNOC_LOG_TRACE(
            "set_rx_frequency(freq=" << (freq / 1e6) << " MHz, chan=" << chan << ")");
        return get_tree()
            ->access<double>(get_db_path("rx", chan) / "freq" / "value")
            .set(freq)
            .get();
    }

    void set_rx_tune_args(const uhd::device_addr_t& tune_args, const size_t chan) override
    {
        if (get_tree()->exists(get_db_path("rx", chan) / "tune_args")) {
            get_tree()
                ->access<uhd::device_addr_t>(get_db_path("rx", chan) / "tune_args")
                .set(tune_args);
        }
    }

    double get_rx_frequency(const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("rx", chan) / "freq" / "value")
            .get();
    }

    uhd::freq_range_t get_tx_frequency_range(const size_t chan) const override
    {
        return get_tree()
            ->access<uhd::freq_range_t>(get_db_path("tx", chan) / "freq" / "range")
            .get();
    }

    uhd::freq_range_t get_rx_frequency_range(const size_t chan) const override
    {
        return get_tree()
            ->access<uhd::meta_range_t>(get_db_path("rx", chan) / "freq" / "range")
            .get();
    }

    /*** Bandwidth-Related APIs************************************************/
    double set_rx_bandwidth(const double bandwidth, const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("rx", chan) / "bandwidth" / "value")
            .set(bandwidth)
            .get();
    }

    double get_rx_bandwidth(const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("rx", chan) / "bandwidth" / "value")
            .get();
    }

    uhd::meta_range_t get_rx_bandwidth_range(size_t chan) const override
    {
        return get_tree()
            ->access<uhd::meta_range_t>(get_db_path("rx", chan) / "bandwidth" / "range")
            .get();
    }

    double set_tx_bandwidth(const double bandwidth, const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("tx", chan) / "bandwidth" / "value")
            .set(bandwidth)
            .get();
    }

    double get_tx_bandwidth(const size_t chan) override
    {
        return get_tree()
            ->access<double>(get_db_path("tx", chan) / "bandwidth" / "value")
            .get();
    }

    uhd::meta_range_t get_tx_bandwidth_range(size_t chan) const override
    {
        return get_tree()
            ->access<uhd::meta_range_t>(get_db_path("tx", chan) / "bandwidth" / "range")
            .get();
    }

    /*** Gain-Related APIs ***************************************************/
    double set_tx_gain(const double gain, const size_t chan) override
    {
        return set_tx_gain(gain, ALL_GAINS, chan);
    }

    double set_tx_gain(
        const double gain, const std::string& name, const size_t chan) override
    {
        _tx_pwr_mgr.at(chan)->set_tracking_mode(pwr_cal_mgr::tracking_mode::TRACK_GAIN);
        if (_tx_gain_groups.count(chan)) {
            auto& gg = _tx_gain_groups.at(chan);
            gg->set_value(gain, name);
            return radio_control_impl::set_tx_gain(gg->get_value(name), chan);
        }
        return radio_control_impl::set_tx_gain(0.0, chan);
    }

    double set_rx_gain(const double gain, const size_t chan) override
    {
        return set_rx_gain(gain, ALL_GAINS, chan);
    }

    double set_rx_gain(
        const double gain, const std::string& name, const size_t chan) override
    {
        _rx_pwr_mgr.at(chan)->set_tracking_mode(pwr_cal_mgr::tracking_mode::TRACK_GAIN);
        auto& gg = _rx_gain_groups.at(chan);
        gg->set_value(gain, name);
        return radio_control_impl::set_rx_gain(gg->get_value(name), chan);
    }

    double get_rx_gain(const size_t chan) override
    {
        return get_rx_gain(ALL_GAINS, chan);
    }

    double get_rx_gain(const std::string& name, const size_t chan) override
    {
        return _rx_gain_groups.at(chan)->get_value(name);
    }

    double get_tx_gain(const size_t chan) override
    {
        return get_tx_gain(ALL_GAINS, chan);
    }

    double get_tx_gain(const std::string& name, const size_t chan) override
    {
        return _tx_gain_groups.at(chan)->get_value(name);
    }

    std::vector<std::string> get_tx_gain_names(const size_t chan) const override
    {
        return _tx_gain_groups.at(chan)->get_names();
    }

    std::vector<std::string> get_rx_gain_names(const size_t chan) const override
    {
        return _rx_gain_groups.at(chan)->get_names();
    }

    uhd::gain_range_t get_tx_gain_range(const size_t chan) const override
    {
        return get_tx_gain_range(ALL_GAINS, chan);
    }

    uhd::gain_range_t get_tx_gain_range(
        const std::string& name, const size_t chan) const override
    {
        if (!_tx_gain_groups.count(chan)) {
            throw uhd::index_error(
                "Trying to access invalid TX gain group: " + std::to_string(chan));
        }
        return _tx_gain_groups.at(chan)->get_range(name);
    }

    uhd::gain_range_t get_rx_gain_range(const size_t chan) const override
    {
        return get_rx_gain_range(ALL_GAINS, chan);
    }

    uhd::gain_range_t get_rx_gain_range(
        const std::string& name, const size_t chan) const override
    {
        if (!_rx_gain_groups.count(chan)) {
            throw uhd::index_error(
                "Trying to access invalid RX gain group: " + std::to_string(chan));
        }
        return _rx_gain_groups.at(chan)->get_range(name);
    }

    std::vector<std::string> get_tx_gain_profile_names(const size_t chan) const override
    {
        return get_tree()
            ->access<std::vector<std::string>>(
                get_db_path("tx", chan) / "gains/all/profile/options")
            .get();
    }

    std::vector<std::string> get_rx_gain_profile_names(const size_t chan) const override
    {
        return get_tree()
            ->access<std::vector<std::string>>(
                get_db_path("rx", chan) / "gains/all/profile/options")
            .get();
    }


    void set_tx_gain_profile(const std::string& profile, const size_t chan) override
    {
        get_tree()
            ->access<std::string>(get_db_path("tx", chan) / "gains/all/profile/value")
            .set(profile);
    }

    void set_rx_gain_profile(const std::string& profile, const size_t chan) override
    {
        get_tree()
            ->access<std::string>(get_db_path("rx", chan) / "gains/all/profile/value")
            .set(profile);
    }


    std::string get_tx_gain_profile(const size_t chan) const override
    {
        return get_tree()
            ->access<std::string>(get_db_path("tx", chan) / "gains/all/profile/value")
            .get();
    }

    std::string get_rx_gain_profile(const size_t chan) const override
    {
        return get_tree()
            ->access<std::string>(get_db_path("rx", chan) / "gains/all/profile/value")
            .get();
    }

    /**************************************************************************
     * LO controls
     *************************************************************************/
    std::vector<std::string> get_rx_lo_names(const size_t chan) const override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);
        std::vector<std::string> lo_names;
        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            for (const std::string& name : get_tree()->list(rx_fe_fe_root / "los")) {
                lo_names.push_back(name);
            }
        }
        return lo_names;
    }

    std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t chan) const override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                if (get_tree()->exists(rx_fe_fe_root / "los" / ALL_LOS)) {
                    // Special value ALL_LOS support atomically sets the source for all
                    // LOs
                    return get_tree()
                        ->access<std::vector<std::string>>(
                            rx_fe_fe_root / "los" / ALL_LOS / "source" / "options")
                        .get();
                } else {
                    return std::vector<std::string>();
                }
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    return get_tree()
                        ->access<std::vector<std::string>>(
                            rx_fe_fe_root / "los" / name / "source" / "options")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s) then it can only be internal
            return std::vector<std::string>(1, "internal");
        }
    }

    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                if (get_tree()->exists(rx_fe_fe_root / "los" / ALL_LOS)) {
                    // Special value ALL_LOS support atomically sets the source for all
                    // LOs
                    get_tree()
                        ->access<std::string>(
                            rx_fe_fe_root / "los" / ALL_LOS / "source" / "value")
                        .set(src);
                } else {
                    for (const std::string& n : get_tree()->list(rx_fe_fe_root / "los")) {
                        this->set_rx_lo_source(src, n, chan);
                    }
                }
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    get_tree()
                        ->access<std::string>(
                            rx_fe_fe_root / "los" / name / "source" / "value")
                        .set(src);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            if (not(src == "internal" and name == ALL_LOS)) {
                throw uhd::runtime_error(
                    "This device only supports setting internal source on all LOs");
            }
        }
    }

    const std::string get_rx_lo_source(
        const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                // Special value ALL_LOS support atomically sets the source for all LOs
                return get_tree()
                    ->access<std::string>(
                        rx_fe_fe_root / "los" / ALL_LOS / "source" / "value")
                    .get();
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    return get_tree()
                        ->access<std::string>(
                            rx_fe_fe_root / "los" / name / "source" / "value")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s) then it can only be internal
            return "internal";
        }
    }

    void set_rx_lo_export_enabled(
        bool enabled, const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                if (get_tree()->exists(rx_fe_fe_root / "los" / ALL_LOS)) {
                    // Special value ALL_LOS support atomically sets the source for all
                    // LOs
                    get_tree()
                        ->access<bool>(rx_fe_fe_root / "los" / ALL_LOS / "export")
                        .set(enabled);
                } else {
                    for (const std::string& n : get_tree()->list(rx_fe_fe_root / "los")) {
                        this->set_rx_lo_export_enabled(enabled, n, chan);
                    }
                }
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    get_tree()
                        ->access<bool>(rx_fe_fe_root / "los" / name / "export")
                        .set(enabled);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            if (not(enabled == false and name == ALL_LOS)) {
                throw uhd::runtime_error("This device only supports setting LO export "
                                         "enabled to false on all LOs");
            }
        }
    }

    bool get_rx_lo_export_enabled(const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                // Special value ALL_LOS support atomically sets the source for all LOs
                return get_tree()
                    ->access<bool>(rx_fe_fe_root / "los" / ALL_LOS / "export")
                    .get();
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    return get_tree()
                        ->access<bool>(rx_fe_fe_root / "los" / name / "export")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s), assume it cannot export
            return false;
        }
    }

    double set_rx_lo_freq(
        double freq, const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error(
                    "LO frequency must be set for each stage individually");
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    get_tree()
                        ->access<double>(rx_fe_fe_root / "los" / name / "freq" / "value")
                        .set(freq);
                    return get_tree()
                        ->access<double>(rx_fe_fe_root / "los" / name / "freq" / "value")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error(
                "This device does not support manual configuration of LOs");
        }
    }

    double get_rx_lo_freq(const std::string& name, const size_t chan) override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error(
                    "LO frequency must be retrieved for each stage individually");
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    return get_tree()
                        ->access<double>(rx_fe_fe_root / "los" / name / "freq" / "value")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return actual RF frequency if the daughterboard doesn't expose its LO(s)
            return get_tree()->access<double>(rx_fe_fe_root / "freq" / " value").get();
        }
    }

    freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override
    {
        fs_path rx_fe_fe_root = get_db_path("rx", chan);

        if (get_tree()->exists(rx_fe_fe_root / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error(
                    "LO frequency range must be retrieved for each stage individually");
            } else {
                if (get_tree()->exists(rx_fe_fe_root / "los")) {
                    return get_tree()
                        ->access<freq_range_t>(
                            rx_fe_fe_root / "los" / name / "freq" / "range")
                        .get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return the actual RF range if the daughterboard doesn't expose its LO(s)
            return get_tree()
                ->access<meta_range_t>(rx_fe_fe_root / "freq" / "range")
                .get();
        }
    }

    /*** Calibration API *****************************************************/
    void set_tx_dc_offset(const std::complex<double>& offset, size_t chan) override
    {
        const fs_path dc_offset_path = get_fe_path("tx", chan) / "dc_offset" / "value";
        if (get_tree()->exists(dc_offset_path)) {
            get_tree()->access<std::complex<double>>(dc_offset_path).set(offset);
        } else {
            RFNOC_LOG_WARNING("Setting TX DC offset is not possible on this device.");
        }
    }

    meta_range_t get_tx_dc_offset_range(size_t chan) const override
    {
        const fs_path range_path = get_fe_path("tx", chan) / "dc_offset" / "range";
        if (get_tree()->exists(range_path)) {
            return get_tree()->access<uhd::meta_range_t>(range_path).get();
        } else {
            RFNOC_LOG_WARNING(
                "This device does not support querying the TX DC offset range.");
            return meta_range_t(0.0, 0.0);
        }
    }

    void set_tx_iq_balance(const std::complex<double>& correction, size_t chan) override
    {
        const fs_path iq_balance_path = get_fe_path("tx", chan) / "iq_balance" / "value";
        if (get_tree()->exists(iq_balance_path)) {
            get_tree()->access<std::complex<double>>(iq_balance_path).set(correction);
        } else {
            RFNOC_LOG_WARNING("Setting TX IQ Balance is not possible on this device.");
        }
    }

    void set_rx_dc_offset(const bool enb, size_t chan) override
    {
        const fs_path dc_offset_path = get_fe_path("rx", chan) / "dc_offset" / "enable";
        if (get_tree()->exists(dc_offset_path)) {
            get_tree()->access<bool>(dc_offset_path).set(enb);
        } else {
            RFNOC_LOG_WARNING(
                "Setting DC offset compensation is not possible on this device.");
        }
    }

    void set_rx_dc_offset(const std::complex<double>& offset, size_t chan) override
    {
        const fs_path dc_offset_path = get_fe_path("rx", chan) / "dc_offset" / "value";
        if (get_tree()->exists(dc_offset_path)) {
            get_tree()->access<std::complex<double>>(dc_offset_path).set(offset);
        } else {
            RFNOC_LOG_WARNING("Setting RX DC offset is not possible on this device.");
        }
    }

    meta_range_t get_rx_dc_offset_range(size_t chan) const override
    {
        const fs_path range_path = get_fe_path("rx", chan) / "dc_offset" / "range";
        if (get_tree()->exists(range_path)) {
            return get_tree()->access<uhd::meta_range_t>(range_path).get();
        } else {
            RFNOC_LOG_WARNING(
                "This device does not support querying the rx DC offset range.");
            return meta_range_t(0.0, 0.0);
        }
    }

    void set_rx_iq_balance(const bool enb, size_t chan) override
    {
        const fs_path iq_balance_path = get_fe_path("rx", chan) / "iq_balance" / "enable";
        if (get_tree()->exists(iq_balance_path)) {
            get_tree()->access<bool>(iq_balance_path).set(enb);
        } else {
            RFNOC_LOG_WARNING(
                "Setting automatic RX IQ Balance is not possible on this device.");
        }
    }

    void set_rx_iq_balance(const std::complex<double>& correction, size_t chan) override
    {
        const fs_path iq_balance_path = get_fe_path("rx", chan) / "iq_balance" / "value";
        if (get_tree()->exists(iq_balance_path)) {
            get_tree()->access<std::complex<double>>(iq_balance_path).set(correction);
        } else {
            RFNOC_LOG_WARNING(
                "Setting manual RX IQ Balance is not possible on this device.");
        }
    }

    /*** GPIO API ************************************************************/
    std::vector<std::string> get_gpio_banks() const override
    {
        return {"FP0", "RX", "TX"};
    }

    void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) override
    {
        if (bank == "FP0") {
            _fp_gpio->set_gpio_attr(usrp::gpio_atr::gpio_attr_rev_map.at(attr), value);
            return;
        }
        if (bank.size() >= 2 and bank[1] == 'X') {
            const std::string name          = bank.substr(2);
            const dboard_iface::unit_t unit = (bank[0] == 'R') ? dboard_iface::UNIT_RX
                                                               : dboard_iface::UNIT_TX;
            constexpr uint16_t mask         = 0xFFFF;
            if (attr == "CTRL") {
                _db_iface->set_pin_ctrl(unit, value, mask);
            } else if (attr == "DDR") {
                _db_iface->set_gpio_ddr(unit, value, mask);
            } else if (attr == "OUT") {
                _db_iface->set_gpio_out(unit, value, mask);
            } else if (attr == "ATR_0X") {
                _db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, value, mask);
            } else if (attr == "ATR_RX") {
                _db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, value, mask);
            } else if (attr == "ATR_TX") {
                _db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, value, mask);
            } else if (attr == "ATR_XX") {
                _db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, value, mask);
            } else {
                RFNOC_LOG_ERROR("Invalid GPIO attribute name: " << attr);
                throw uhd::key_error(std::string("Invalid GPIO attribute name: ") + attr);
            }
            return;
        }
        RFNOC_LOG_WARNING(
            "Invalid GPIO bank name: `"
            << bank
            << "'. Ignoring call to set_gpio_attr() to retain backward compatibility.");
    }

    uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) override
    {
        if (bank == "FP0") {
            return _fp_gpio->get_attr_reg(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
        }
        if (bank.size() >= 2 and bank[1] == 'X') {
            const std::string name          = bank.substr(2);
            const dboard_iface::unit_t unit = (bank[0] == 'R') ? dboard_iface::UNIT_RX
                                                               : dboard_iface::UNIT_TX;
            if (attr == "CTRL")
                return _db_iface->get_pin_ctrl(unit);
            if (attr == "DDR")
                return _db_iface->get_gpio_ddr(unit);
            if (attr == "OUT")
                return _db_iface->get_gpio_out(unit);
            if (attr == "ATR_0X")
                return _db_iface->get_atr_reg(unit, gpio_atr::ATR_REG_IDLE);
            if (attr == "ATR_RX")
                return _db_iface->get_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY);
            if (attr == "ATR_TX")
                return _db_iface->get_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY);
            if (attr == "ATR_XX")
                return _db_iface->get_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX);
            if (attr == "READBACK")
                return _db_iface->read_gpio(unit);
            RFNOC_LOG_ERROR("Invalid GPIO attribute name: " << attr);
            throw uhd::key_error(std::string("Invalid GPIO attribute name: ") + attr);
        }
        RFNOC_LOG_WARNING(
            "Invalid GPIO bank name: `"
            << bank
            << "'. get_gpio_attr() will return 0 to retain backward compatibility.");
        return 0;
    }

    /**************************************************************************
     * Sensor API
     *************************************************************************/
    std::vector<std::string> get_rx_sensor_names(size_t chan) const override
    {
        const fs_path sensor_path = get_db_path("rx", chan) / "sensors";
        if (get_tree()->exists(sensor_path)) {
            return get_tree()->list(sensor_path);
        }
        return {};
    }

    uhd::sensor_value_t get_rx_sensor(const std::string& name, size_t chan) override
    {
        return get_tree()
            ->access<uhd::sensor_value_t>(get_db_path("rx", chan) / "sensors" / name)
            .get();
    }

    std::vector<std::string> get_tx_sensor_names(size_t chan) const override
    {
        const fs_path sensor_path = get_db_path("tx", chan) / "sensors";
        if (get_tree()->exists(sensor_path)) {
            return get_tree()->list(sensor_path);
        }
        return {};
    }

    uhd::sensor_value_t get_tx_sensor(const std::string& name, size_t chan) override
    {
        return get_tree()
            ->access<uhd::sensor_value_t>(get_db_path("tx", chan) / "sensors" / name)
            .get();
    }

    /**************************************************************************
     * EEPROM API
     *************************************************************************/
    void set_db_eeprom(const uhd::eeprom_map_t& db_eeprom) override
    {
        const std::string key_prefix = db_eeprom.count("rx_id") ? "rx_" : "tx_";
        const std::string id_key     = key_prefix + "id";
        const std::string serial_key = key_prefix + "serial";
        const std::string rev_key    = key_prefix + "rev";
        if (!(db_eeprom.count(id_key) && db_eeprom.count(serial_key)
                && db_eeprom.count(rev_key))) {
            RFNOC_LOG_ERROR("set_db_eeprom() requires id, serial, and rev keys!");
            throw uhd::key_error(
                "[X300] set_db_eeprom() requires id, serial, and rev keys!");
        }

        dboard_eeprom_t eeprom;
        eeprom.id.from_string(bytes_to_str(db_eeprom.at(id_key)));
        eeprom.serial   = bytes_to_str(db_eeprom.at(serial_key));
        eeprom.revision = bytes_to_str(db_eeprom.at(rev_key));
        if (get_tree()->exists(DB_PATH / (key_prefix + "eeprom"))) {
            get_tree()
                ->access<dboard_eeprom_t>(DB_PATH / (key_prefix + "eeprom"))
                .set(eeprom);
        } else {
            RFNOC_LOG_WARNING("Cannot set EEPROM, tree path does not exist.");
        }
    }


    uhd::eeprom_map_t get_db_eeprom() override
    {
        uhd::eeprom_map_t result;
        if (get_tree()->exists(DB_PATH / "rx_eeprom")) {
            const auto rx_eeprom =
                get_tree()->access<dboard_eeprom_t>(DB_PATH / "rx_eeprom").get();
            result["rx_id"]     = str_to_bytes(rx_eeprom.id.to_pp_string());
            result["rx_serial"] = str_to_bytes(rx_eeprom.serial);
            result["rx_rev"]    = str_to_bytes(rx_eeprom.revision);
        }
        if (get_tree()->exists(DB_PATH / "tx_eeprom")) {
            const auto tx_eeprom =
                get_tree()->access<dboard_eeprom_t>(DB_PATH / "tx_eeprom").get();
            result["tx_id"]     = str_to_bytes(tx_eeprom.id.to_pp_string());
            result["tx_serial"] = str_to_bytes(tx_eeprom.serial);
            result["tx_rev"]    = str_to_bytes(tx_eeprom.revision);
        }
        return result;
    }

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/
    std::string get_slot_name() const override
    {
        return _radio_type == PRIMARY ? "A" : "B";
    }

    size_t get_chan_from_dboard_fe(
        const std::string& fe, const uhd::direction_t direction) const override
    {
        switch (direction) {
            case uhd::TX_DIRECTION:
                return _get_chan_from_map(_tx_fe_map, fe);
            case uhd::RX_DIRECTION:
                return _get_chan_from_map(_rx_fe_map, fe);
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    std::string get_dboard_fe_from_chan(
        const size_t chan, const uhd::direction_t direction) const override
    {
        switch (direction) {
            case uhd::TX_DIRECTION:
                return _tx_fe_map.at(chan).db_fe_name;
            case uhd::RX_DIRECTION:
                return _rx_fe_map.at(chan).db_fe_name;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    std::string get_fe_name(
        const size_t chan, const uhd::direction_t direction) const override
    {
        fs_path name_path =
            get_db_path(direction == uhd::RX_DIRECTION ? "rx" : "tx", chan) / "name";
        if (!get_tree()->exists(name_path)) {
            return get_dboard_fe_from_chan(chan, direction);
        }

        return get_tree()->access<std::string>(name_path).get();
    }


    void set_command_time(uhd::time_spec_t time, const size_t chan) override
    {
        node_t::set_command_time(time, chan);
        // This is for TwinRX only:
        fs_path cmd_time_path = get_db_path("rx", chan) / "time" / "cmd";
        if (get_tree()->exists(cmd_time_path)) {
            get_tree()->access<time_spec_t>(cmd_time_path).set(time);
        }
    }

    /**************************************************************************
     * MB Interface API Calls
     *************************************************************************/
    uint32_t get_adc_rx_word() override
    {
        return regs().peek32(regmap::RADIO_BASE_ADDR + regmap::REG_RX_DATA);
    }

    void set_adc_test_word(
        const std::string& patterna, const std::string& patternb) override
    {
        _adc->set_test_word(patterna, patternb);
    }

    void set_adc_checker_enabled(const bool enb) override
    {
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, enb ? 1 : 0);
    }

    bool get_adc_checker_locked(const bool I) override
    {
        return bool(_regs->misc_ins_reg.read(
            I ? radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED
              : radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED));
    }

    uint32_t get_adc_checker_error_code(const bool I) override
    {
        return _regs->misc_ins_reg.get(
            I ? radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR
              : radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR);
    }

    // Documented in x300_radio_mbc_iface.hpp
    void self_test_adc(const uint32_t ramp_time_ms) override
    {
        RFNOC_LOG_DEBUG("Running ADC self-cal...");
        // Bypass all front-end corrections
        for (size_t i = 0; i < get_num_output_ports(); i++) {
            _rx_fe_map[i].core->bypass_all(true);
        }

        // Test basic patterns
        _adc->set_test_word("ones", "ones");
        _check_adc(0xfffcfffc);
        _adc->set_test_word("zeros", "zeros");
        _check_adc(0x00000000);
        _adc->set_test_word("ones", "zeros");
        _check_adc(0xfffc0000);
        _adc->set_test_word("zeros", "ones");
        _check_adc(0x0000fffc);
        for (size_t k = 0; k < 14; k++) {
            _adc->set_test_word("zeros", "custom", 1 << k);
            _check_adc(1 << (k + 2));
        }
        for (size_t k = 0; k < 14; k++) {
            _adc->set_test_word("custom", "zeros", 1 << k);
            _check_adc(1 << (k + 18));
        }

        // Turn on ramp pattern test
        _adc->set_test_word("ramp", "ramp");
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
        // Sleep added for SPI transactions to finish and ramp to start before checker is
        // enabled.
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);

        std::this_thread::sleep_for(std::chrono::milliseconds(ramp_time_ms));
        _regs->misc_ins_reg.refresh();

        std::string i_status, q_status;
        if (_regs->misc_ins_reg.get(
                radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED))
            if (_regs->misc_ins_reg.get(
                    radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR))
                i_status = "Bit Errors!";
            else
                i_status = "Good";
        else
            i_status = "Not Locked!";

        if (_regs->misc_ins_reg.get(
                radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED))
            if (_regs->misc_ins_reg.get(
                    radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR))
                q_status = "Bit Errors!";
            else
                q_status = "Good";
        else
            q_status = "Not Locked!";

        // Return to normal mode
        _adc->set_test_word("normal", "normal");

        if ((i_status != "Good") or (q_status != "Good")) {
            throw uhd::runtime_error(
                (boost::format("ADC self-test failed for %s. Ramp checker status: "
                               "{ADC_A=%s, ADC_B=%s}")
                    % get_unique_id() % i_status % q_status)
                    .str());
        }

        // Restore front-end corrections
        for (size_t i = 0; i < get_num_output_ports(); i++) {
            _rx_fe_map[i].core->bypass_all(false);
        }
    }

    void sync_dac() override
    {
        _dac->sync();
    }

    void set_dac_sync(const bool enb, const uhd::time_spec_t& time) override
    {
        if (time != uhd::time_spec_t(0.0)) {
            set_command_time(time, 0);
        }
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::DAC_SYNC, enb ? 1 : 0);
        if (!enb && time != uhd::time_spec_t(0.0)) {
            set_command_time(uhd::time_spec_t(0.0), 0);
        }
    }

    void dac_verify_sync() override
    {
        _dac->verify_sync();
    }

private:
    /**************************************************************************
     * ADC Control
     *************************************************************************/
    //! Create the ADC/DAC objects, reset them, run ADC cal
    void _init_codecs()
    {
        _regs = std::make_unique<radio_regmap_t>(get_block_id().get_block_count());
        _regs->initialize(*_wb_iface, true);
        // Only Radio0 has the ADC/DAC reset bits
        if (_radio_type == PRIMARY) {
            RFNOC_LOG_TRACE("Resetting DAC and ADCs...");
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
            _regs->misc_outs_reg.flush();
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 0);
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 1);
            _regs->misc_outs_reg.flush();
        }
        _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::DAC_ENABLED, 1);

        RFNOC_LOG_TRACE("Creating ADC interface...");
        _adc = x300_adc_ctrl::make(_spi, DB_ADC_SEN);
        RFNOC_LOG_TRACE("Creating DAC interface...");
        _dac = x300_dac_ctrl::make(_spi, DB_DAC_SEN, _master_clock_rate);
        _self_cal_adc_capture_delay();

        ////////////////////////////////////////////////////////////////
        // create legacy codec control objects
        ////////////////////////////////////////////////////////////////
        // DAC has no gains
        get_tree()->create<int>("tx_codec/gains");
        get_tree()->create<std::string>("tx_codec/name").set("ad9146");
        get_tree()->create<std::string>("rx_codec/name").set("ads62p48");
        get_tree()
            ->create<meta_range_t>("rx_codec/gains/digital/range")
            .set(meta_range_t(0, 6.0, 0.5));
        get_tree()
            ->create<double>("rx_codec/gains/digital/value")
            .add_coerced_subscriber([this](const double gain) { _adc->set_gain(gain); })
            .set(0);
    }

    //! Calibrate delays on the ADC. This needs to happen before every session.
    void _self_cal_adc_capture_delay()
    {
        RFNOC_LOG_TRACE("Running ADC capture delay self-cal...");
        constexpr uint32_t NUM_DELAY_STEPS = 32; // The IDELAYE2 element has 32 steps
        // Retry self-cal if it fails in warmup situations
        constexpr uint32_t NUM_RETRIES   = 2;
        constexpr int32_t MIN_WINDOW_LEN = 4;

        int32_t win_start = -1, win_stop = -1;
        uint32_t iter = 0;
        while (iter++ < NUM_RETRIES) {
            for (uint32_t dly_tap = 0; dly_tap < NUM_DELAY_STEPS; dly_tap++) {
                // Apply delay
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, dly_tap);
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

                uint32_t err_code = 0;

                // -- Test I Channel --
                // Put ADC in ramp test mode. Tie the other channel to all ones.
                _adc->set_test_word("ramp", "ones");
                // Turn on the pattern checker in the FPGA. It will lock when it sees a
                // zero and count deviations from the expected value
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
                // 5ms @ 200MHz = 1 million samples
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                if (_regs->misc_ins_reg.read(
                        radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_LOCKED)) {
                    err_code += _regs->misc_ins_reg.get(
                        radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_ERROR);
                } else {
                    err_code += 100; // Increment error code by 100 to indicate no lock
                }

                // -- Test Q Channel --
                // Put ADC in ramp test mode. Tie the other channel to all ones.
                _adc->set_test_word("ones", "ramp");
                // Turn on the pattern checker in the FPGA. It will lock when it sees a
                // zero and count deviations from the expected value
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
                _regs->misc_outs_reg.write(
                    radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
                // 5ms @ 200MHz = 1 million samples
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                if (_regs->misc_ins_reg.read(
                        radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_LOCKED)) {
                    err_code += _regs->misc_ins_reg.get(
                        radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_ERROR);
                } else {
                    err_code += 100; // Increment error code by 100 to indicate no lock
                }

                if (err_code == 0) {
                    if (win_start == -1) { // This is the first window
                        win_start = dly_tap;
                        win_stop  = dly_tap;
                    } else { // We are extending the window
                        win_stop = dly_tap;
                    }
                } else {
                    if (win_start != -1) { // A valid window turned invalid
                        if (win_stop - win_start >= MIN_WINDOW_LEN) {
                            break; // Valid window found
                        } else {
                            win_start = -1; // Reset window
                        }
                    }
                }
                // UHD_LOGGER_INFO("X300 RADIO") << (boost::format("CapTap=%d, Error=%d")
                // % dly_tap % err_code);
            }

            // Retry the self-cal if it fails
            if ((win_start == -1 || (win_stop - win_start) < MIN_WINDOW_LEN)
                && iter < NUM_RETRIES /*not last iteration*/) {
                win_start = -1;
                win_stop  = -1;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            } else {
                break;
            }
        }
        _adc->set_test_word("normal", "normal");
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);

        if (win_start == -1) {
            throw uhd::runtime_error("self_cal_adc_capture_delay: Self calibration "
                                     "failed. Convergence error.");
        }

        if (win_stop - win_start < MIN_WINDOW_LEN) {
            throw uhd::runtime_error(
                "self_cal_adc_capture_delay: Self calibration failed. "
                "Valid window too narrow.");
        }

        uint32_t ideal_tap = (win_stop + win_start) / 2;
        _regs->misc_outs_reg.write(
            radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, ideal_tap);
        _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
        _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

        double tap_delay = (1.0e12 / 200e6) / (2 * 32); // in ps
        RFNOC_LOG_DEBUG(
            boost::format("ADC capture delay self-cal done (Tap=%d, Window=%d, "
                          "TapDelay=%.3fps, Iter=%d)")
            % ideal_tap % (win_stop - win_start) % tap_delay % iter);
    }

    //! Verify that the output of the ADC matches an expected \p val
    void _check_adc(const uint32_t val)
    {
        // Wait for previous control transaction to flush
        get_adc_rx_word();
        // Wait for ADC test pattern to propagate
        std::this_thread::sleep_for(std::chrono::microseconds(5));
        // Read value of RX readback register and verify, adapt for I inversion
        // in FPGA
        const uint32_t adc_rb = get_adc_rx_word() ^ 0xfffc0000;
        if (val != adc_rb) {
            RFNOC_LOG_ERROR(boost::format("ADC self-test failed! (Exp=0x%x, Got=0x%x)")
                            % val % adc_rb);
            throw uhd::runtime_error("ADC self-test failed!");
        }
    }

    void reset_codec()
    {
        RFNOC_LOG_TRACE("Start reset_codec");
        if (_radio_type == PRIMARY) { // ADC/DAC reset lines only exist in Radio0
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
            _regs->misc_outs_reg.flush();
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 0);
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 1);
            _regs->misc_outs_reg.flush();
        }
        _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::DAC_ENABLED, 1);
        UHD_ASSERT_THROW(bool(_adc));
        UHD_ASSERT_THROW(bool(_dac));
        _adc->reset();
        _dac->reset();
        RFNOC_LOG_TRACE("Done reset_codec");
    }

    /**************************************************************************
     * DBoard
     *************************************************************************/
    fs_path get_db_path(const std::string& dir, const size_t chan) const
    {
        UHD_ASSERT_THROW(dir == "rx" || dir == "tx");
        if (dir == "rx" && chan >= get_num_output_ports()) {
            throw uhd::key_error("Invalid RX channel: " + std::to_string(chan));
        }
        if (dir == "tx" && chan >= get_num_input_ports()) {
            throw uhd::key_error("Invalid TX channel: " + std::to_string(chan));
        }
        return DB_PATH / (dir + "_frontends")
               / ((dir == "rx") ? _rx_fe_map.at(chan).db_fe_name
                                : _tx_fe_map.at(chan).db_fe_name);
    }

    fs_path get_fe_path(const std::string& dir, const size_t chan) const
    {
        UHD_ASSERT_THROW(dir == "rx" || dir == "tx");
        if (dir == "rx" && chan >= get_num_output_ports()) {
            throw uhd::key_error("Invalid RX channel: " + std::to_string(chan));
        }
        if (dir == "tx" && chan >= get_num_input_ports()) {
            throw uhd::key_error("Invalid TX channel: " + std::to_string(chan));
        }
        return FE_PATH / (dir + "_fe_corrections")
               / ((dir == "rx") ? _rx_fe_map.at(chan).db_fe_name
                                : _tx_fe_map.at(chan).db_fe_name);
    }

    void _init_db()
    {
        constexpr size_t BASE_ADDR       = 0x50;
        constexpr size_t RX_EEPROM_ADDR  = 0x5;
        constexpr size_t TX_EEPROM_ADDR  = 0x4;
        constexpr size_t GDB_EEPROM_ADDR = 0x1;
        static const std::vector<size_t> EEPROM_ADDRS{
            RX_EEPROM_ADDR, TX_EEPROM_ADDR, GDB_EEPROM_ADDR};
        static const std::vector<std::string> EEPROM_PATHS{
            "rx_eeprom", "tx_eeprom", "gdb_eeprom"};
        const size_t DB_OFFSET = (_radio_type == PRIMARY) ? 0x0 : 0x2;
        auto zpu_i2c           = _x300_mb_control->get_zpu_i2c();
        auto clock             = _x300_mb_control->get_clock_ctrl();
        for (size_t i = 0; i < EEPROM_ADDRS.size(); i++) {
            const size_t addr = EEPROM_ADDRS[i] + DB_OFFSET;
            // Load EEPROM
            _db_eeproms[addr].load(*zpu_i2c, BASE_ADDR | addr);
            // Use the RFNoC implementation for Basic/LF dboards
            uint16_t dboard_pid = _db_eeproms[addr].id.to_uint16();
            switch (dboard_pid) {
                case uhd::usrp::dboard::basic_and_lf::BASIC_RX_PID:
                case uhd::usrp::dboard::basic_and_lf::LF_RX_PID:
                    dboard_pid |= uhd::usrp::dboard::basic_and_lf::RFNOC_PID_FLAG;
                    _db_eeproms[addr].id = dboard_pid;
                    _basic_lf_rx         = true;
                    break;
                case uhd::usrp::dboard::basic_and_lf::BASIC_TX_PID:
                case uhd::usrp::dboard::basic_and_lf::LF_TX_PID:
                    dboard_pid |= uhd::usrp::dboard::basic_and_lf::RFNOC_PID_FLAG;
                    _db_eeproms[addr].id = dboard_pid;
                    _basic_lf_tx         = true;
                    break;
            }
            // Add to tree
            get_tree()
                ->create<dboard_eeprom_t>(DB_PATH / EEPROM_PATHS[i])
                .set(_db_eeproms[addr])
                .add_coerced_subscriber([this, zpu_i2c, BASE_ADDR, addr](
                                            const uhd::usrp::dboard_eeprom_t& db_eeprom) {
                    _set_db_eeprom(zpu_i2c, BASE_ADDR | addr, db_eeprom);
                });
        }

        // create a new dboard interface
        x300_dboard_iface_config_t db_config;
        db_config.gpio           = gpio_atr::db_gpio_atr_3000::make(_wb_iface,
            gpio_atr::gpio_atr_offsets::make_default(x300_regs::SR_DB_GPIO,
                x300_regs::RB_DB_GPIO,
                x300_regs::PERIPH_REG_OFFSET));
        db_config.spi            = _spi;
        db_config.rx_spi_slaveno = DB_RX_SEN;
        db_config.tx_spi_slaveno = DB_TX_SEN;
        db_config.i2c            = zpu_i2c;
        db_config.clock          = clock;
        db_config.which_rx_clk   = (_radio_type == PRIMARY) ? X300_CLOCK_WHICH_DB0_RX
                                                            : X300_CLOCK_WHICH_DB1_RX;
        db_config.which_tx_clk   = (_radio_type == PRIMARY) ? X300_CLOCK_WHICH_DB0_TX
                                                            : X300_CLOCK_WHICH_DB1_TX;
        db_config.dboard_slot    = (_radio_type == PRIMARY) ? 0 : 1;
        db_config.cmd_time_ctrl  = _wb_iface;

        // create a new dboard manager
        RFNOC_LOG_TRACE("Creating DB interface...");
        _db_iface = std::make_shared<x300_dboard_iface>(db_config);
        RFNOC_LOG_TRACE("Creating DB manager...");
        _db_manager = dboard_manager::make(_db_eeproms[RX_EEPROM_ADDR + DB_OFFSET],
            _db_eeproms[TX_EEPROM_ADDR + DB_OFFSET],
            _db_eeproms[GDB_EEPROM_ADDR + DB_OFFSET],
            _db_iface,
            get_tree()->subtree(DB_PATH),
            true // defer daughterboard initialization
        );
        RFNOC_LOG_TRACE("DB Manager Initialization complete.");

        // The X3x0 radio block defaults to a maximum of two ports, but
        // many daughterboards have fewer possible frontends. So we now
        // reduce the number of actual ports based on what is connected.
        // Note: The Basic and LF boards have two possible frontends on
        // the rx side, and one on the tx side. TwinRX boards have two
        // possible rx frontends, and require up to two ports on the rx side.
        // For all other cases, the number of possible frontends is one for
        // rx and tx.
        const size_t num_tx_frontends = _db_manager->get_tx_frontends().size();
        const size_t num_rx_frontends = _db_manager->get_rx_frontends().size();
        if (num_tx_frontends == 2 || num_tx_frontends == 1) {
            set_num_input_ports(num_tx_frontends);
        } else {
            throw uhd::runtime_error("Unexpected number of TX frontends!");
        }
        if (num_rx_frontends == 2 || num_rx_frontends == 1) {
            set_num_output_ports(num_rx_frontends);
        } else {
            throw uhd::runtime_error("Unexpected number of RX frontends!");
        }
        // This is specific to TwinRX. Due to driver legacy, we think we have a
        // Tx frontend even though we don't. We thus hard-code that knowledge
        // here.
        if (num_rx_frontends == 2
            && boost::starts_with(
                get_tree()->access<std::string>(DB_PATH / "rx_frontends/0/name").get(),
                "TwinRX")) {
            _twinrx = true;
            set_num_input_ports(0);
        }
        RFNOC_LOG_TRACE("Num Active Frontends: RX: " << get_num_output_ports()
                                                     << " TX: " << get_num_input_ports());
    } // _init_db()

    void _init_dboards()
    {
        size_t rx_chan = 0;
        size_t tx_chan = 0;
        for (const std::string& fe : _db_manager->get_rx_frontends()) {
            if (rx_chan >= get_num_output_ports()) {
                break;
            }
            _set_rx_fe(fe, rx_chan);
            rx_chan++;
        }
        for (const std::string& fe : _db_manager->get_tx_frontends()) {
            if (tx_chan >= get_num_input_ports()) {
                break;
            }
            _set_tx_fe(fe, tx_chan);
            tx_chan++;
        }
        UHD_ASSERT_THROW(rx_chan or tx_chan);
        RFNOC_LOG_DEBUG("Actual sample rate: " << (get_rate() / 1e6) << " Msps.");

        // Initialize the daughterboards now that frontend cores and connections exist
        _db_manager->initialize_dboards();

        // now that dboard is created -- register into rx antenna event
        if (not _rx_fe_map.empty()) {
            for (size_t i = 0; i < get_num_output_ports(); i++) {
                if (get_tree()->exists(get_db_path("rx", i) / "antenna" / "value")) {
                    // We need a desired subscriber for antenna/value because the experts
                    // don't coerce that property.
                    get_tree()
                        ->access<std::string>(get_db_path("rx", i) / "antenna" / "value")
                        .add_desired_subscriber([this, i](const std::string& led) {
                            _update_atr_leds(led, i);
                        })
                        .update();
                } else {
                    _update_atr_leds("", i); // init anyway, even if never called
                }
            }
        }

        // bind frontend corrections to the dboard freq props
        if (not _tx_fe_map.empty()) {
            for (size_t i = 0; i < get_num_input_ports(); i++) {
                if (get_tree()->exists(get_db_path("tx", i) / "freq" / "value")) {
                    get_tree()
                        ->access<double>(get_db_path("tx", i) / "freq" / "value")
                        .add_coerced_subscriber([this, i](const double freq) {
                            set_tx_fe_corrections(freq, i);
                        });
                }
            }
        }
        if (not _rx_fe_map.empty()) {
            for (size_t i = 0; i < get_num_output_ports(); i++) {
                if (get_tree()->exists(get_db_path("rx", i) / "freq" / "value")) {
                    get_tree()
                        ->access<double>(get_db_path("rx", i) / "freq" / "value")
                        .add_coerced_subscriber([this, i](const double freq) {
                            set_rx_fe_corrections(freq, i);
                        });
                }
            }
        }

        ////////////////////////////////////////////////////////////////
        // Set gain groups
        // Note: The actual gain control comes from the daughterboard drivers, thus,
        // we need to call into the prop tree at the appropriate location in order
        // to modify the gains.
        ////////////////////////////////////////////////////////////////
        // TX
        for (size_t chan = 0; chan < get_num_input_ports(); chan++) {
            fs_path rf_gains_path(get_db_path("tx", chan) / "gains");
            if (!get_tree()->exists(rf_gains_path)) {
                _tx_gain_groups[chan] = gain_group::make_zero();
                continue;
            }

            std::vector<std::string> gain_stages = get_tree()->list(rf_gains_path);
            if (gain_stages.empty()) {
                _tx_gain_groups[chan] = gain_group::make_zero();
                continue;
            }

            // DAC does not have a gain path
            auto gg = gain_group::make();
            for (const auto& name : gain_stages) {
                gg->register_fcns(name,
                    make_gain_fcns_from_subtree(
                        get_tree()->subtree(rf_gains_path / name)),
                    1 /* high prio */);
            }
            _tx_gain_groups[chan] = gg;
        }
        // RX
        for (size_t chan = 0; chan < get_num_output_ports(); chan++) {
            fs_path rf_gains_path(get_db_path("rx", chan) / "gains");
            fs_path adc_gains_path("rx_codec/gains");

            auto gg = gain_group::make();
            // ADC also has a gain path
            for (const auto& name : get_tree()->list(adc_gains_path)) {
                gg->register_fcns("ADC-" + name,
                    make_gain_fcns_from_subtree(
                        get_tree()->subtree(adc_gains_path / name)),
                    0 /* low prio */);
            }
            if (get_tree()->exists(rf_gains_path)) {
                for (const auto& name : get_tree()->list(rf_gains_path)) {
                    gg->register_fcns(name,
                        make_gain_fcns_from_subtree(
                            get_tree()->subtree(rf_gains_path / name)),
                        1 /* high prio */);
                }
            }
            _rx_gain_groups[chan] = gg;
        }

        ////////////////////////////////////////////////////////////////
        // Load calibration data
        ////////////////////////////////////////////////////////////////
        RFNOC_LOG_TRACE("Initializing power calibration data...");
        // RX and TX are symmetric, so we use a macro to avoid some duplication
#define INIT_POWER_CAL(dir)                                                             \
    {                                                                                   \
        const std::string DIR  = (#dir == std::string("tx")) ? "TX" : "RX";             \
        const size_t num_ports = (#dir == std::string("tx")) ? get_num_input_ports()    \
                                                             : get_num_output_ports();  \
        _##dir##_pwr_mgr.resize(num_ports);                                             \
        for (size_t chan = 0; chan < num_ports; chan++) {                               \
            const auto eeprom =                                                         \
                get_tree()->access<dboard_eeprom_t>(DB_PATH / (#dir "_eeprom")).get();  \
            /* The cal serial is the daughterboard serial plus the FE name */           \
            const std::string cal_serial =                                              \
                eeprom.serial + "#" + _##dir##_fe_map.at(chan).db_fe_name;              \
            /* Now create a gain group for this. _?x_gain_groups won't work, */         \
            /* unfortunately, because it doesn't group the gains we want them to */     \
            /* be grouped. */                                                           \
            auto ggroup = uhd::gain_group::make();                                      \
            ggroup->register_fcns(HW_GAIN_STAGE,                                        \
                {[this, chan]() { return get_##dir##_gain_range(chan); },               \
                    [this, chan]() { return get_##dir##_gain(chan); },                  \
                    [this, chan](const double gain) { set_##dir##_gain(gain, chan); }}, \
                10 /* High priority */);                                                \
            /* If we had a digital (baseband) gain, we would register it here, so */    \
            /* that the power manager would know to use it as a backup gain stage. */   \
            _##dir##_pwr_mgr.at(chan) = pwr_cal_mgr::make(                              \
                cal_serial,                                                             \
                "X300-CAL-" + DIR,                                                      \
                [this, chan]() { return get_##dir##_frequency(chan); },                 \
                [this, chan]() -> std::string {                                         \
                    const auto id_path = get_db_path(#dir, chan) / "id";                \
                    const std::string db_suffix =                                       \
                        get_tree()->exists(id_path)                                     \
                            ? get_tree()->access<std::string>(id_path).get()            \
                            : "generic";                                                \
                    const std::string ant = get_##dir##_antenna(chan);                  \
                    return "x3xx_pwr_" + db_suffix + "_" + #dir + "_"                   \
                           + pwr_cal_mgr::sanitize_antenna_name(ant);                   \
                },                                                                      \
                ggroup);                                                                \
            /* Every time we retune, we need to re-set the power level, if */           \
            /* we're in power tracking mode */                                          \
            get_tree()                                                                  \
                ->access<double>(get_db_path(#dir, chan) / "freq" / "value")            \
                .add_coerced_subscriber([this, chan](const double) {                    \
                    _##dir##_pwr_mgr.at(chan)->update_power();                          \
                });                                                                     \
        }                                                                               \
    } // end macro

        INIT_POWER_CAL(tx);
        INIT_POWER_CAL(rx);
    } /* _init_dboards */

    void _set_db_eeprom(i2c_iface::sptr i2c,
        const size_t addr,
        const uhd::usrp::dboard_eeprom_t& db_eeprom)
    {
        db_eeprom.store(*i2c, addr);
        _db_eeproms[addr] = db_eeprom;
    }

    // A note on updating the LED ATR register: There is a single ATR register
    // for the radio block, despite there being 2 channels. For most (1-channel)
    // daughterboards, the rules are simple: When transmitting, the red LED turns
    // on. When receiving, either the green LED under the RX2 or on the TX/RX
    // port turn, depending on if the user has selected a TX/RX antenna or not.
    // For TwinRX, we have additional rules. The board has two channels, but
    // either of them can used with any SMA port. We therefore have to check
    // which channels are active (0, 1, or both) and on the active channels,
    // which set of antenna ports is used (RX1 aka TX/RX, RX2). All active RX
    // ports shall then be added the the RX ATR register.
    void _update_atr_leds(const std::string& rx_ant, const size_t /*chan*/)
    {
        uint32_t rx_led_atr_state = 0;
        if (_twinrx) {
            for (size_t chan = 0; chan < get_num_output_ports(); chan++) {
                const auto fe_enable_path = get_db_path("rx", chan) / "enabled";
                if (get_tree()->access<bool>(fe_enable_path).get()) {
                    if (get_rx_antenna(chan) == "RX1") {
                        rx_led_atr_state |= x300_regs::SR_LED_TXRX_RX;
                    }
                    if (get_rx_antenna(chan) == "RX2") {
                        rx_led_atr_state |= x300_regs::SR_LED_RX2_RX;
                    }
                }
            }
        } else {
            rx_led_atr_state = rx_ant == "TX/RX" ? x300_regs::SR_LED_TXRX_RX
                                                 : x300_regs::SR_LED_RX2_RX;
        }
        _leds->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, rx_led_atr_state);
    }

    void _set_rx_fe(const std::string& fe, const size_t chan)
    {
        _rx_fe_map[chan].db_fe_name = fe;
        _db_iface->add_rx_fe(fe, _rx_fe_map[chan].core);
        const std::string connection =
            get_tree()->access<std::string>(get_db_path("rx", chan) / "connection").get();
        const double if_freq =
            (get_tree()->exists(get_db_path("rx", chan) / "if_freq" / "value"))
                ? get_tree()
                      ->access<double>(get_db_path("rx", chan) / "if_freq" / "value")
                      .get()
                : 0.0;
        _rx_fe_map[chan].core->set_fe_connection(
            usrp::fe_connection_t(connection, if_freq));
    }

    void _set_tx_fe(const std::string& fe, const size_t chan)
    {
        _tx_fe_map[chan].db_fe_name = fe;
        const std::string connection =
            get_tree()->access<std::string>(get_db_path("tx", chan) / "connection").get();
        _tx_fe_map[chan].core->set_mux(connection);
    }

    void set_rx_fe_corrections(const double lo_freq, const size_t chan)
    {
        if (not _ignore_cal_file) {
            apply_rx_fe_corrections(get_tree(),
                get_tree()->access<dboard_eeprom_t>(DB_PATH / "rx_eeprom").get().serial,
                get_fe_path("rx", chan),
                lo_freq);
        }
    }

    void set_tx_fe_corrections(const double lo_freq, const size_t chan)
    {
        if (not _ignore_cal_file) {
            apply_tx_fe_corrections(get_tree(),
                get_tree()->access<dboard_eeprom_t>(DB_PATH / "tx_eeprom").get().serial,
                get_fe_path("tx", chan),
                lo_freq);
        }
    }

    /**************************************************************************
     * noc_block_base API
     *************************************************************************/
    //! Safely shut down all peripherals
    //
    // Reminder: After this is called, no peeks and pokes are allowed!
    void deinit() override
    {
        RFNOC_LOG_TRACE("deinit()");
        // Reset daughterboard
        _db_manager.reset();
        _db_iface.reset();
        // Reset codecs
        if (_radio_type == PRIMARY) {
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
            _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
        }
        _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::DAC_ENABLED, 0);
        _regs->misc_outs_reg.flush();
        _adc.reset();
        _dac.reset();
        // Destroy all other periph controls
        _spi.reset();
        _fp_gpio.reset();
        _leds.reset();
        _rx_fe_map.clear();
        _tx_fe_map.clear();
    }

    bool check_topology(const std::vector<size_t>& connected_inputs,
        const std::vector<size_t>& connected_outputs) override
    {
        RFNOC_LOG_TRACE("check_topology()");
        if (!node_t::check_topology(connected_inputs, connected_outputs)) {
            return false;
        }

        for (size_t chan = 0; chan < get_num_input_ports(); chan++) {
            const auto fe_enable_path = get_db_path("tx", chan) / "enabled";
            if (get_tree()->exists(fe_enable_path)) {
                const bool chan_active = std::any_of(connected_inputs.cbegin(),
                    connected_inputs.cend(),
                    [chan](const size_t input) { return input == chan; });
                RFNOC_LOG_TRACE(
                    "Enabling TX chan " << chan << ": " << (chan_active ? "Yes" : "No"));
                get_tree()->access<bool>(fe_enable_path).set(chan_active);
            }
        }

        for (size_t chan = 0; chan < get_num_output_ports(); chan++) {
            const auto fe_enable_path = get_db_path("rx", chan) / "enabled";
            if (get_tree()->exists(fe_enable_path)) {
                const bool chan_active = std::any_of(connected_outputs.cbegin(),
                    connected_outputs.cend(),
                    [chan](const size_t output) { return output == chan; });
                RFNOC_LOG_TRACE(
                    "Enabling RX chan " << chan << ": " << (chan_active ? "Yes" : "No"));
                get_tree()->access<bool>(fe_enable_path).set(chan_active);
            }
            // Modifying the number of active channels can affect how the
            // front-panel LEDs get configured for TwinRX boards. Worst case,
            // this is a no-op since we call it with the same argument as it was
            // called before. Note this must be called after the 'enable'
            // property is set.
            _update_atr_leds(get_rx_antenna(chan), chan);
        }

        return true;
    }


    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Register space for the ADC and DAC
    class radio_regmap_t : public uhd::soft_regmap_t
    {
    public:
        class misc_outs_reg_t : public uhd::soft_reg32_wo_t
        {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(DAC_ENABLED, /*width*/ 1, /*shift*/ 0); //[0]
            UHD_DEFINE_SOFT_REG_FIELD(DAC_RESET_N, /*width*/ 1, /*shift*/ 1); //[1]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_RESET, /*width*/ 1, /*shift*/ 2); //[2]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_DATA_DLY_STB, /*width*/ 1, /*shift*/ 3); //[3]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_DATA_DLY_VAL, /*width*/ 5, /*shift*/ 4); //[8:4]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER_ENABLED, /*width*/ 1, /*shift*/ 9); //[9]
            UHD_DEFINE_SOFT_REG_FIELD(DAC_SYNC, /*width*/ 1, /*shift*/ 10); //[10]

            misc_outs_reg_t() : uhd::soft_reg32_wo_t(x300_regs::SR_MISC_OUTS)
            {
                // Initial values
                set(DAC_ENABLED, 0);
                set(DAC_RESET_N, 0);
                set(ADC_RESET, 0);
                set(ADC_DATA_DLY_STB, 0);
                set(ADC_DATA_DLY_VAL, 16);
                set(ADC_CHECKER_ENABLED, 0);
                set(DAC_SYNC, 0);
            }
        } misc_outs_reg;

        class misc_ins_reg_t : public uhd::soft_reg64_ro_t
        {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER0_Q_LOCKED, /*width*/ 1, /*shift*/ 32); //[0]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER0_I_LOCKED, /*width*/ 1, /*shift*/ 33); //[1]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER1_Q_LOCKED, /*width*/ 1, /*shift*/ 34); //[2]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER1_I_LOCKED, /*width*/ 1, /*shift*/ 35); //[3]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER0_Q_ERROR, /*width*/ 1, /*shift*/ 36); //[4]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER0_I_ERROR, /*width*/ 1, /*shift*/ 37); //[5]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER1_Q_ERROR, /*width*/ 1, /*shift*/ 38); //[6]
            UHD_DEFINE_SOFT_REG_FIELD(
                ADC_CHECKER1_I_ERROR, /*width*/ 1, /*shift*/ 39); //[7]

            misc_ins_reg_t() : uhd::soft_reg64_ro_t(x300_regs::RB_MISC_IO) {}
        } misc_ins_reg;

        radio_regmap_t(int radio_num)
            : soft_regmap_t("radio" + std::to_string(radio_num) + "_regmap")
        {
            add_to_map(misc_outs_reg, "misc_outs_reg", PRIVATE);
            add_to_map(misc_ins_reg, "misc_ins_reg", PRIVATE);
        }
    }; /* class radio_regmap_t */
    //! wb_iface Instance for _regs
    uhd::timed_wb_iface::sptr _wb_iface;
    //! Instantiation of regs object for ADC and DAC (MISC_OUT register)
    std::unique_ptr<radio_regmap_t> _regs;
    //! Reference to the MB controller, typecast
    std::shared_ptr<x300_mb_controller> _x300_mb_control;

    //! Reference to the DBoard SPI core (also controls ADC/DAC)
    spi_core_3000::sptr _spi;
    //! Reference to the ADC controller
    x300_adc_ctrl::sptr _adc;
    //! Reference to the DAC controller
    x300_dac_ctrl::sptr _dac;
    //! Front-panel GPIO
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;
    //! LEDs
    usrp::gpio_atr::gpio_atr_3000::sptr _leds;

    struct rx_fe_perif
    {
        std::string name;
        std::string db_fe_name;
        rx_frontend_core_3000::sptr core;
    };
    struct tx_fe_perif
    {
        std::string name;
        std::string db_fe_name;
        tx_frontend_core_200::sptr core;
    };

    bool _basic_lf_rx = false;
    bool _basic_lf_tx = false;
    bool _twinrx      = false;

    std::unordered_map<size_t, rx_fe_perif> _rx_fe_map;
    std::unordered_map<size_t, tx_fe_perif> _tx_fe_map;

    //! Cache of EEPROM info (one per channel)
    std::unordered_map<size_t, usrp::dboard_eeprom_t> _db_eeproms;
    //! Reference to DB manager
    usrp::dboard_manager::sptr _db_manager;
    //! Reference to DB iface
    std::shared_ptr<x300_dboard_iface> _db_iface;

    enum radio_connection_t { PRIMARY, SECONDARY };
    radio_connection_t _radio_type;

    bool _ignore_cal_file = false;

    std::unordered_map<size_t, uhd::gain_group::sptr> _tx_gain_groups;
    std::unordered_map<size_t, uhd::gain_group::sptr> _rx_gain_groups;

    double _master_clock_rate = DEFAULT_RATE;
};

UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    x300_radio_control, RADIO_BLOCK, X300, "Radio", true, "radio_clk", "radio_clk")
