//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_constants.hpp"
#include "e3xx_radio_control_impl.hpp"
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::rfnoc;

void e3xx_radio_control_impl::_init_defaults()
{
    RFNOC_LOG_TRACE("Initializing defaults...");
    const size_t num_rx_chans = get_num_output_ports();
    const size_t num_tx_chans = get_num_input_ports();

    RFNOC_LOG_TRACE(
        "Num TX chans: " << num_tx_chans << " Num RX chans: " << num_rx_chans);


    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    auto block_args = get_block_args();
    _master_clock_rate =
        _rpcc->request_with_token<double>(_rpc_prefix + "get_master_clock_rate");
    const double block_args_mcr =
        block_args.cast<double>("master_clock_rate", _master_clock_rate);
    if (block_args_mcr != _master_clock_rate) {
        throw uhd::runtime_error(
            str(boost::format("Master clock rate mismatch. Device returns %f MHz, "
                              "but should have been %f MHz.")
                % (_master_clock_rate / 1e6) % (block_args_mcr / 1e6)));
    }
    RFNOC_LOG_DEBUG("Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    set_tick_rate(_master_clock_rate);
    _e3xx_timekeeper->update_tick_rate(_master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);
    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_control_impl::set_rx_frequency(E3XX_DEFAULT_FREQ, chan);
        radio_control_impl::set_rx_gain(E3XX_DEFAULT_GAIN, chan);
        radio_control_impl::set_rx_antenna(E3XX_DEFAULT_RX_ANTENNA, chan);
        radio_control_impl::set_rx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_control_impl::set_tx_frequency(E3XX_DEFAULT_FREQ, chan);
        radio_control_impl::set_tx_gain(E3XX_DEFAULT_GAIN, chan);
        radio_control_impl::set_tx_antenna(E3XX_DEFAULT_TX_ANTENNA, chan);
        radio_control_impl::set_tx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
    }

    _rx_sensor_names = _rpcc->request_with_token<std::vector<std::string>>(
        this->_rpc_prefix + "get_sensors", "RX");
    _tx_sensor_names = _rpcc->request_with_token<std::vector<std::string>>(
        this->_rpc_prefix + "get_sensors", "TX");

    // Cache the filter names
    // FIXME: Uncomment this
    //_rx_filter_names = _ad9361->get_filter_names(
    //    get_which_ad9361_chain(RX_DIRECTION, 0, _fe_swap));
    //_tx_filter_names = _ad9361->get_filter_names(
    //    get_which_ad9361_chain(TX_DIRECTION, 0, _fe_swap));
}

void e3xx_radio_control_impl::_init_peripherals()
{
    RFNOC_LOG_TRACE("Initializing peripherals...");
    for (size_t radio_idx = 0; radio_idx < E3XX_NUM_CHANS; radio_idx++) {
        _wb_ifaces.push_back(RFNOC_MAKE_WB_IFACE(0, radio_idx));
    }
    _db_gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < E3XX_NUM_CHANS; radio_idx++) {
        RFNOC_LOG_TRACE("Initializing DB GPIOs for channel " << radio_idx);
        // Note: The register offset is baked into the different _wb_iface
        // objects!
        _db_gpio.emplace_back(
            usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.at(radio_idx),
                usrp::gpio_atr::gpio_atr_offsets::make_write_only(
                    e3xx_regs::SR_DB_GPIO + (radio_idx * e3xx_regs::PERIPH_REG_CHAN_OFFSET),
                    e3xx_regs::PERIPH_REG_OFFSET)));
        _db_gpio[radio_idx]->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    }
    _leds_gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < E3XX_NUM_CHANS; radio_idx++) {
        RFNOC_LOG_TRACE("Initializing LED GPIOs for channel " << radio_idx);
        _leds_gpio.emplace_back(
            usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.at(radio_idx),
                usrp::gpio_atr::gpio_atr_offsets::make_write_only(
                    e3xx_regs::SR_LEDS + (radio_idx * e3xx_regs::PERIPH_REG_CHAN_OFFSET),
                    e3xx_regs::PERIPH_REG_OFFSET)));
        _leds_gpio[radio_idx]->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    }
    RFNOC_LOG_TRACE("Initializing front-panel GPIO control...")
    _fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.at(0),
        usrp::gpio_atr::gpio_atr_offsets::make_default(
            e3xx_regs::SR_FP_GPIO,
            e3xx_regs::RB_FP_GPIO,
            e3xx_regs::PERIPH_REG_OFFSET));


    auto block_args = get_block_args();
    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration          = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = 5;
        }
        _identify_with_leds(identify_duration);
    }
}

void e3xx_radio_control_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree, const size_t chan_idx)
{
    const fs_path tx_fe_path = fs_path("tx_frontends") / chan_idx;
    const fs_path rx_fe_path = fs_path("rx_frontends") / chan_idx;
    RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel "
                    << chan_idx << " to prop tree path " << tx_fe_path << " and "
                    << rx_fe_path);
    // TX Standard attributes
    subtree->create<std::string>(tx_fe_path / "name").set("E3xx");
    subtree->create<std::string>(tx_fe_path / "connection").set("IQ");
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name").set("E3xx");
    subtree->create<std::string>(rx_fe_path / "connection").set("IQ");
    // TX Antenna
    subtree->create<std::string>(tx_fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string& ant) {
            this->set_tx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_antenna(chan_idx); });
    subtree->create<std::vector<std::string>>(tx_fe_path / "antenna" / "options")
        .set({E3XX_DEFAULT_TX_ANTENNA})
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
        .set(E3XX_RX_ANTENNAS)
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
        .set_publisher([this]() { return get_tx_frequency_range(0); })
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
        .set_publisher([this]() { return get_rx_frequency_range(0); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set_publisher([this, chan_idx]() { return get_tx_bandwidth(chan_idx); })
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_tx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_bandwidth(chan_idx); });
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set_publisher([this]() { return get_tx_bandwidth_range(0); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        });
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set_publisher([this, chan_idx]() { return get_rx_bandwidth(chan_idx); })
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_rx_bandwidth(bw, chan_idx);
        });
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .set_publisher([this]() { return get_rx_bandwidth_range(0); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        });

    // TX gains
    const std::vector<std::string> tx_gain_names = ad9361_ctrl::get_gain_names("TX1");
    for (auto tx_gain_name : tx_gain_names) {
        subtree->create<double>(tx_fe_path / "gains" / tx_gain_name / "value")
            .set_coercer([this, chan_idx](const double gain) {
                return this->set_tx_gain(gain, chan_idx);
            })
            .set_publisher(
                [this, chan_idx]() { return radio_control_impl::get_tx_gain(chan_idx); });
        subtree->create<meta_range_t>(tx_fe_path / "gains" / tx_gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([]() {
                return meta_range_t(
                    AD9361_MIN_TX_GAIN, AD9361_MAX_TX_GAIN, AD9361_TX_GAIN_STEP);
            });
    }

    // RX gains
    const std::vector<std::string> rx_gain_names = ad9361_ctrl::get_gain_names("RX1");
    for (auto rx_gain_name : rx_gain_names) {
        subtree->create<double>(rx_fe_path / "gains" / rx_gain_name / "value")
            .set_coercer([this, chan_idx](const double gain) {
                return this->set_rx_gain(gain, chan_idx);
            })
            .set_publisher(
                [this, chan_idx]() { return radio_control_impl::get_rx_gain(chan_idx); });

        subtree->create<meta_range_t>(rx_fe_path / "gains" / rx_gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([]() {
                return meta_range_t(
                    AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN, AD9361_RX_GAIN_STEP);
            });
    }

    auto rx_sensor_names = get_rx_sensor_names(chan_idx);
    for (const auto& rx_sensor_name : rx_sensor_names) {
        RFNOC_LOG_TRACE("Adding RX sensor " << rx_sensor_name);
        get_tree()
            ->create<sensor_value_t>(rx_fe_path / "sensors" / rx_sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, rx_sensor_name, chan_idx]() {
                return get_rx_sensor(rx_sensor_name, chan_idx);
            });
    }
    auto tx_sensor_names = get_tx_sensor_names(chan_idx);
    for (const auto& tx_sensor_name : tx_sensor_names) {
        RFNOC_LOG_TRACE("Adding TX sensor " << tx_sensor_name);
        get_tree()
            ->create<sensor_value_t>(tx_fe_path / "sensors" / tx_sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, tx_sensor_name, chan_idx]() {
                return get_tx_sensor(tx_sensor_name, chan_idx);
            });
    }
}

void e3xx_radio_control_impl::_init_prop_tree()
{
    for (size_t chan_idx = 0; chan_idx < E3XX_NUM_CHANS; chan_idx++) {
        this->_init_frontend_subtree(get_tree()->subtree(DB_PATH), chan_idx);
    }
    get_tree()->create<std::string>("rx_codec/name").set("AD9361 Dual ADC");
    get_tree()->create<std::string>("tx_codec/name").set("AD9361 Dual DAC");
}

void e3xx_radio_control_impl::_init_mpm()
{
    // Initialize catalina
    _init_codec();

    // Loopback test
    for (size_t chan = 0; chan < E3XX_NUM_CHANS; chan++) {
        loopback_self_test(chan);
    }
}

void e3xx_radio_control_impl::_init_codec()
{
    RFNOC_LOG_TRACE("Setting Catalina Defaults... ");
    for (size_t chan = 0; chan < E3XX_NUM_CHANS; chan++) {
        std::string rx_fe = get_which_ad9361_chain(RX_DIRECTION, chan);
        this->set_rx_gain(E3XX_DEFAULT_GAIN, chan);
        this->set_rx_frequency(E3XX_DEFAULT_FREQ, chan);
        this->set_rx_antenna(E3XX_DEFAULT_RX_ANTENNA, chan);
        this->set_rx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
        _ad9361->set_dc_offset_auto(rx_fe, E3XX_DEFAULT_AUTO_DC_OFFSET);
        _ad9361->set_iq_balance_auto(rx_fe, E3XX_DEFAULT_AUTO_IQ_BALANCE);
        _ad9361->set_agc(rx_fe, E3XX_DEFAULT_AGC_ENABLE);
        std::string tx_fe = get_which_ad9361_chain(TX_DIRECTION, chan);
        this->set_tx_gain(E3XX_DEFAULT_GAIN, chan);
        this->set_tx_frequency(E3XX_DEFAULT_FREQ, chan);
        this->set_tx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
    }
}
