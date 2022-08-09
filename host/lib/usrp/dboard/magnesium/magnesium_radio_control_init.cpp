//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_constants.hpp"
#include "magnesium_radio_control.hpp"
#include <uhd/types/eeprom.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::rfnoc;

namespace {

enum slave_select_t { SEN_CPLD = 1, SEN_TX_LO = 2, SEN_RX_LO = 4, SEN_PHASE_DAC = 8 };

constexpr double MAGNESIUM_DEFAULT_FREQ      = 2.5e9; // Hz
constexpr double MAGNESIUM_DEFAULT_BANDWIDTH = 100e6; // Hz

} // namespace

void magnesium_radio_control_impl::_init_defaults()
{
    RFNOC_LOG_TRACE("_init_defaults()");
    for (size_t chan = 0; chan < get_num_output_ports(); chan++) {
        radio_control_impl::set_rx_frequency(MAGNESIUM_DEFAULT_FREQ, chan);
        radio_control_impl::set_rx_gain(0, chan);
        radio_control_impl::set_rx_antenna(MAGNESIUM_DEFAULT_RX_ANTENNA, chan);
        radio_control_impl::set_rx_bandwidth(MAGNESIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < get_num_input_ports(); chan++) {
        radio_control_impl::set_tx_frequency(MAGNESIUM_DEFAULT_FREQ, chan);
        radio_control_impl::set_tx_gain(0, chan);
        radio_control_impl::set_tx_antenna(MAGNESIUM_DEFAULT_TX_ANTENNA, chan);
        radio_control_impl::set_tx_bandwidth(MAGNESIUM_DEFAULT_BANDWIDTH, chan);
    }

    const auto block_args = get_block_args();
    if (block_args.has_key("tx_gain_profile")) {
        RFNOC_LOG_INFO("Using user specified TX gain profile: " << block_args.get(
                           "tx_gain_profile"));
        set_tx_gain_profile(block_args.get("tx_gain_profile"), 0);
    }

    if (block_args.has_key("rx_gain_profile")) {
        RFNOC_LOG_INFO("Using user specified RX gain profile: " << block_args.get(
                           "rx_gain_profile"));
        set_rx_gain_profile(block_args.get("rx_gain_profile"), 0);
    }

    if (block_args.has_key("rx_band_map")) {
        RFNOC_LOG_INFO("Using user specified RX band limits");
        _remap_band_limits(block_args.get("rx_band_map"), RX_DIRECTION);
    }

    if (block_args.has_key("tx_band_map")) {
        RFNOC_LOG_INFO("Using user specified TX band limits");
        _remap_band_limits(block_args.get("tx_band_map"), TX_DIRECTION);
    }
}

void magnesium_radio_control_impl::_init_peripherals()
{
    RFNOC_LOG_TRACE("Initializing peripherals...");
    RFNOC_LOG_TRACE("Initializing SPI core...");
    _spi = spi_core_3000::make(
        [this](uint32_t addr, uint32_t data) {
            regs().poke32(addr, data, get_command_time(0));
        },
        [this](uint32_t addr) { return regs().peek32(addr, get_command_time(0)); },
        n310_regs::SR_SPI,
        8,
        n310_regs::RB_SPI);
    RFNOC_LOG_TRACE("Initializing CPLD...");
    RFNOC_LOG_TRACE("Creating new CPLD object...");
    spi_config_t spi_config;
    spi_config.use_custom_divider = true;
    spi_config.divider            = 125;
    spi_config.mosi_edge          = spi_config_t::EDGE_RISE;
    spi_config.miso_edge          = spi_config_t::EDGE_FALL;
    RFNOC_LOG_TRACE("Making CPLD object...");
    _cpld = std::make_shared<magnesium_cpld_ctrl>(
        [this, spi_config](const uint32_t transaction) { // Write functor
            this->_spi->write_spi(SEN_CPLD, spi_config, transaction, 24);
        },
        [this, spi_config](const uint32_t transaction) { // Read functor
            return this->_spi->read_spi(SEN_CPLD, spi_config, transaction, 24);
        });
    _update_atr_switches(
        magnesium_cpld_ctrl::BOTH, DX_DIRECTION, radio_control_impl::get_rx_antenna(0));
    RFNOC_LOG_TRACE("Initializing TX LO...");
    _tx_lo = adf435x_iface::make_adf4351([this](
                                             const std::vector<uint32_t> transactions) {
        for (const uint32_t transaction : transactions) {
            this->_spi->write_spi(SEN_TX_LO, spi_config_t::EDGE_RISE, transaction, 32);
        }
    });
    RFNOC_LOG_TRACE("Initializing RX LO...");
    _rx_lo = adf435x_iface::make_adf4351([this](
                                             const std::vector<uint32_t> transactions) {
        for (const uint32_t transaction : transactions) {
            this->_spi->write_spi(SEN_RX_LO, spi_config_t::EDGE_RISE, transaction, 32);
        }
    });

    _gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < get_num_input_ports(); radio_idx++) {
        _wb_ifaces.push_back(RFNOC_MAKE_WB_IFACE(0, radio_idx));
        RFNOC_LOG_TRACE("Initializing GPIOs for channel " << radio_idx);
        _gpio.emplace_back(usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.back(),
            usrp::gpio_atr::gpio_atr_offsets::make_default(
                n310_regs::SR_DB_GPIO + radio_idx * n310_regs::CHAN_REG_OFFSET,
                n310_regs::RB_DB_GPIO + radio_idx * n310_regs::CHAN_REG_OFFSET,
                n310_regs::PERIPH_REG_OFFSET)));
        // DSA and AD9371 gain bits do *not* toggle on ATR modes. If we ever
        // connect anything else to this core, we might need to set_atr_mode()
        // to MODE_ATR on those bits. For now, all bits simply do what they're
        // told, and don't toggle on RX/TX state changes.
        _gpio.back()->set_atr_mode(usrp::gpio_atr::MODE_GPIO, // Disable ATR mode
            usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
        _gpio.back()->set_gpio_ddr(usrp::gpio_atr::DDR_OUTPUT, // Make all GPIOs outputs
            usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    }
    RFNOC_LOG_TRACE("Initializing front-panel GPIO control...")
    _fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.front(),
        usrp::gpio_atr::gpio_atr_offsets::make_default(
            n310_regs::SR_FP_GPIO,
            n310_regs::RB_FP_GPIO,
            n310_regs::PERIPH_REG_OFFSET));
}

void magnesium_radio_control_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree, const size_t chan_idx)
{
    const fs_path tx_fe_path = fs_path("tx_frontends") / chan_idx;
    const fs_path rx_fe_path = fs_path("rx_frontends") / chan_idx;
    RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel "
                    << chan_idx << " to prop tree path " << tx_fe_path << " and "
                    << rx_fe_path);
    // TX Standard attributes
    subtree->create<std::string>(tx_fe_path / "name")
        .set(get_fe_name(chan_idx, TX_DIRECTION));
    subtree->create<std::string>(tx_fe_path / "connection").set("IQ");
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name")
        .set(get_fe_name(chan_idx, RX_DIRECTION));
    subtree->create<std::string>(rx_fe_path / "connection").set("IQ");
    // TX Antenna
    subtree->create<std::string>(tx_fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string& ant) {
            this->set_tx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_antenna(chan_idx); });
    subtree->create<std::vector<std::string>>(tx_fe_path / "antenna" / "options")
        .set_publisher([this]() { return get_tx_antennas(0); })
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
    // RX Antenna
    subtree->create<std::string>(rx_fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string& ant) {
            this->set_rx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_rx_antenna(chan_idx); });
    subtree->create<std::vector<std::string>>(rx_fe_path / "antenna" / "options")
        .set_publisher([this]() { return get_rx_antennas(0); })
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
    // TX frequency
    subtree->create<double>(tx_fe_path / "freq" / "value")
        .set_coercer([this, chan_idx](const double freq) {
            return this->set_tx_frequency(freq, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_frequency(chan_idx); });
    subtree->create<meta_range_t>(tx_fe_path / "freq" / "range")
        .set_publisher([this, chan_idx]() { return get_tx_frequency_range(chan_idx); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
    // RX frequency
    subtree->create<double>(rx_fe_path / "freq" / "value")
        .set_coercer([this, chan_idx](const double freq) {
            return this->set_rx_frequency(freq, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_rx_frequency(chan_idx); });
    subtree->create<meta_range_t>(rx_fe_path / "freq" / "range")
        .set_publisher([this, chan_idx]() { return get_rx_frequency_range(chan_idx); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set(AD9371_TX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_tx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_bandwidth(chan_idx); });
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set_publisher([this, chan_idx]() { return get_tx_bandwidth_range(chan_idx); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        });
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set(AD9371_RX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_rx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_rx_bandwidth(chan_idx); });
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .set_publisher([this, chan_idx]() { return get_rx_bandwidth_range(chan_idx); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        });

    // TX gains
    std::vector<std::string> tx_gain_names = get_tx_gain_names(chan_idx);
    tx_gain_names.push_back("all");
    for (const auto& gain_name : tx_gain_names) {
        subtree->create<double>(tx_fe_path / "gains" / gain_name / "value")
            .set_coercer([this, chan_idx, gain_name](const double gain) {
                return this->set_tx_gain(gain, gain_name, chan_idx);
            })
            .set_publisher([this, chan_idx, gain_name]() {
                return get_tx_gain(gain_name, chan_idx);
            });
        subtree->create<meta_range_t>(tx_fe_path / "gains" / gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([this, gain_name, chan_idx]() {
                return get_tx_gain_range(gain_name, chan_idx);
            });
    }
    subtree->create<std::vector<std::string>>(tx_fe_path / "gains/all/profile/options")
        .set_publisher(
            [this, chan_idx]() { return get_tx_gain_profile_names(chan_idx); });
    subtree->create<std::string>(tx_fe_path / "gains/all/profile/value")
        .set_coercer([this, chan_idx](const std::string& profile) {
            set_tx_gain_profile(profile, chan_idx);
            return profile;
        })
        .set_publisher([this, chan_idx]() { return get_tx_gain_profile(chan_idx); });

    // RX gains
    std::vector<std::string> rx_gain_names = get_rx_gain_names(chan_idx);
    rx_gain_names.push_back("all");
    for (const auto& gain_name : rx_gain_names) {
        subtree->create<double>(rx_fe_path / "gains" / gain_name / "value")
            .set_coercer([this, chan_idx, gain_name](const double gain) {
                return this->set_rx_gain(gain, gain_name, chan_idx);
            })
            .set_publisher([this, chan_idx, gain_name]() {
                return get_rx_gain(gain_name, chan_idx);
            });
        subtree->create<meta_range_t>(rx_fe_path / "gains" / gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([this, gain_name, chan_idx]() {
                return get_rx_gain_range(gain_name, chan_idx);
            });
    }
    subtree->create<std::vector<std::string>>(rx_fe_path / "gains/all/profile/options")
        .set_publisher(
            [this, chan_idx]() { return get_rx_gain_profile_names(chan_idx); });
    subtree->create<std::string>(rx_fe_path / "gains/all/profile/value")
        .set_coercer([this, chan_idx](const std::string& profile) {
            set_rx_gain_profile(profile, chan_idx);
            return profile;
        })
        .set_publisher([this, chan_idx]() { return get_rx_gain_profile(chan_idx); });

    // LO Specific
    // RX LO
    subtree->create<meta_range_t>(rx_fe_path / "los" / MAGNESIUM_LO1 / "freq/range")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_freq_range(MAGNESIUM_LO1, chan_idx);
        });
    subtree
        ->create<std::vector<std::string>>(
            rx_fe_path / "los" / MAGNESIUM_LO1 / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_sources(MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<std::string>(rx_fe_path / "los" / MAGNESIUM_LO1 / "source/value")
        .add_coerced_subscriber([this, chan_idx](std::string src) {
            this->set_rx_lo_source(src, MAGNESIUM_LO1, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_source(MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<double>(rx_fe_path / "los" / MAGNESIUM_LO1 / "freq/value")
        .set_publisher(
            [this, chan_idx]() { return this->get_rx_lo_freq(MAGNESIUM_LO1, chan_idx); })
        .set_coercer([this, chan_idx](const double freq) {
            return this->set_rx_lo_freq(freq, MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<meta_range_t>(rx_fe_path / "los" / MAGNESIUM_LO2 / "freq/range")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_freq_range(MAGNESIUM_LO2, chan_idx);
        });
    subtree
        ->create<std::vector<std::string>>(
            rx_fe_path / "los" / MAGNESIUM_LO2 / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_sources(MAGNESIUM_LO2, chan_idx);
        });

    subtree->create<std::string>(rx_fe_path / "los" / MAGNESIUM_LO2 / "source/value")
        .add_coerced_subscriber([this, chan_idx](std::string src) {
            this->set_rx_lo_source(src, MAGNESIUM_LO2, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_source(MAGNESIUM_LO2, chan_idx);
        });
    subtree->create<double>(rx_fe_path / "los" / MAGNESIUM_LO2 / "freq/value")
        .set_publisher(
            [this, chan_idx]() { return this->get_rx_lo_freq(MAGNESIUM_LO2, chan_idx); })
        .set_coercer([this, chan_idx](double freq) {
            return this->set_rx_lo_freq(freq, MAGNESIUM_LO2, chan_idx);
        });
    // TX LO
    subtree->create<meta_range_t>(tx_fe_path / "los" / MAGNESIUM_LO1 / "freq/range")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_freq_range(MAGNESIUM_LO1, chan_idx);
        });
    subtree
        ->create<std::vector<std::string>>(
            tx_fe_path / "los" / MAGNESIUM_LO1 / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_sources(MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<std::string>(tx_fe_path / "los" / MAGNESIUM_LO1 / "source/value")
        .add_coerced_subscriber([this, chan_idx](std::string src) {
            this->set_tx_lo_source(src, MAGNESIUM_LO1, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_source(MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<double>(tx_fe_path / "los" / MAGNESIUM_LO1 / "freq/value ")
        .set_publisher(
            [this, chan_idx]() { return this->get_tx_lo_freq(MAGNESIUM_LO1, chan_idx); })
        .set_coercer([this, chan_idx](double freq) {
            return this->set_tx_lo_freq(freq, MAGNESIUM_LO1, chan_idx);
        });
    subtree->create<meta_range_t>(tx_fe_path / "los" / MAGNESIUM_LO2 / "freq/range")
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_freq_range(MAGNESIUM_LO2, chan_idx);
        });
    subtree
        ->create<std::vector<std::string>>(
            tx_fe_path / "los" / MAGNESIUM_LO2 / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_sources(MAGNESIUM_LO2, chan_idx);
        });
    subtree->create<std::string>(tx_fe_path / "los" / MAGNESIUM_LO2 / "source/value")
        .add_coerced_subscriber([this, chan_idx](std::string src) {
            this->set_tx_lo_source(src, MAGNESIUM_LO2, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_source(MAGNESIUM_LO2, chan_idx);
        });
    subtree->create<double>(tx_fe_path / "los" / MAGNESIUM_LO2 / "freq/value")
        .set_publisher(
            [this, chan_idx]() { return this->get_tx_lo_freq(MAGNESIUM_LO2, chan_idx); })
        .set_coercer([this, chan_idx](double freq) {
            return this->set_tx_lo_freq(freq, MAGNESIUM_LO2, chan_idx);
        });

    // Sensors
    auto rx_sensor_names = get_rx_sensor_names(chan_idx);
    for (const auto& sensor_name : rx_sensor_names) {
        RFNOC_LOG_TRACE("Adding RX sensor " << sensor_name);
        get_tree()
            ->create<sensor_value_t>(rx_fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, sensor_name, chan_idx]() {
                return get_rx_sensor(sensor_name, chan_idx);
            });
    }
    auto tx_sensor_names = get_tx_sensor_names(chan_idx);
    for (const auto& sensor_name : tx_sensor_names) {
        RFNOC_LOG_TRACE("Adding TX sensor " << sensor_name);
        get_tree()
            ->create<sensor_value_t>(tx_fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, sensor_name, chan_idx]() {
                return get_tx_sensor(sensor_name, chan_idx);
            });
    }
}

void magnesium_radio_control_impl::_init_prop_tree()
{
    for (size_t chan_idx = 0; chan_idx < MAGNESIUM_NUM_CHANS; chan_idx++) {
        this->_init_frontend_subtree(get_tree()->subtree(DB_PATH), chan_idx);
    }

    // DB EEPROM
    get_tree()
        ->create<eeprom_map_t>("eeprom")
        .add_coerced_subscriber(
            [this](const eeprom_map_t& db_eeprom) { set_db_eeprom(db_eeprom); })
        .set_publisher([this]() { return get_db_eeprom(); });
}

void magnesium_radio_control_impl::_init_mpm()
{
    auto block_args = get_block_args();
    RFNOC_LOG_TRACE("Instantiating AD9371 control object...");
    _ad9371 = magnesium_ad9371_iface::uptr(
        new magnesium_ad9371_iface(_rpcc, (_radio_slot == "A") ? 0 : 1));

    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration          = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = 5;
        }
        RFNOC_LOG_INFO("Running LED identification process for " << identify_duration
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
    RFNOC_LOG_DEBUG("Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    set_tick_rate(_master_clock_rate);
    _n3xx_timekeeper->update_tick_rate(_master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);
}
