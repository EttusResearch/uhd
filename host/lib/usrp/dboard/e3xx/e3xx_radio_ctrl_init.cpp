//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_constants.hpp"
#include "e3xx_radio_ctrl_impl.hpp"
#include <uhd/transport/chdr.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::rfnoc;

//! Helper function to extract single value of port number.
//
// Each GPIO pins can be controlled by each radio output ports.
// This function convert the format of attribute "Radio_N_M"
// to a single value port number = N*number_of_port_per_radio + M

uint32_t _extract_port_number(
    std::string radio_src_string, uhd::property_tree::sptr ptree)
{
    std::string s_val = "0";
    std::vector<std::string> radio_strings;
    boost::algorithm::split(radio_strings,
        radio_src_string,
        boost::is_any_of("_/"),
        boost::token_compress_on);
    boost::to_lower(radio_strings[0]);
    if (radio_strings.size() < 3) {
        throw uhd::runtime_error(str(
            boost::format("%s is an invalid GPIO source string.") % radio_src_string));
    }
    size_t radio_num = std::stoi(radio_strings[1]);
    size_t port_num  = std::stoi(radio_strings[2]);
    if (radio_strings[0] != "radio") {
        throw uhd::runtime_error(
            "Front panel GPIO bank can only accept a radio block as its driver.");
    }
    std::string radio_port_out  = "Radio_" + radio_strings[1] + "/ports/out";
    std::string radio_port_path = radio_port_out + "/" + radio_strings[2];
    auto found                  = ptree->exists(fs_path("xbar") / radio_port_path);
    if (not found) {
        throw uhd::runtime_error(
            str(boost::format("Could not find radio port %s.\n") % radio_port_path));
    }
    size_t port_size = ptree->list(fs_path("xbar") / radio_port_out).size();
    return radio_num * port_size + port_num;
}

void e3xx_radio_ctrl_impl::_init_defaults()
{
    UHD_LOG_TRACE(unique_id(), "Initializing defaults...");
    const size_t num_rx_chans = get_output_ports().size();
    const size_t num_tx_chans = get_input_ports().size();

    UHD_LOG_TRACE(unique_id(),
        "Num TX chans: " << num_tx_chans << " Num RX chans: " << num_rx_chans);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(E3XX_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(E3XX_DEFAULT_GAIN, chan);
        radio_ctrl_impl::set_rx_antenna(E3XX_DEFAULT_RX_ANTENNA, chan);
        radio_ctrl_impl::set_rx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_ctrl_impl::set_tx_frequency(E3XX_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_tx_gain(E3XX_DEFAULT_GAIN, chan);
        radio_ctrl_impl::set_tx_antenna(E3XX_DEFAULT_TX_ANTENNA, chan);
        radio_ctrl_impl::set_tx_bandwidth(E3XX_DEFAULT_BANDWIDTH, chan);
    }

    /** Update default SPP (overwrites the default value from the XML file) **/
    const size_t max_bytes_header =
        uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t);
    const size_t default_spp =
        (_tree->access<size_t>("mtu/recv").get() - max_bytes_header)
        / (2 * sizeof(int16_t));
    UHD_LOG_DEBUG(unique_id(), "Setting default spp to " << default_spp);
    _tree->access<int>(get_arg_path("spp") / "value").set(default_spp);
}

void e3xx_radio_ctrl_impl::_init_peripherals()
{
    UHD_LOG_TRACE(unique_id(), "Initializing peripherals...");

    _db_gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < _get_num_radios(); radio_idx++) {
        UHD_LOG_TRACE(unique_id(), "Initializing GPIOs for channel " << radio_idx);
        _db_gpio.emplace_back(usrp::gpio_atr::gpio_atr_3000::make_write_only(
            _get_ctrl(radio_idx), regs::sr_addr(regs::GPIO)));
        _db_gpio[radio_idx]->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    }
    _leds_gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < _get_num_radios(); radio_idx++) {
        UHD_LOG_TRACE(unique_id(), "Initializing GPIOs for channel " << radio_idx);
        _leds_gpio.emplace_back(usrp::gpio_atr::gpio_atr_3000::make_write_only(
            _get_ctrl(radio_idx), regs::sr_addr(regs::LEDS)));

        _leds_gpio[radio_idx]->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    }
    UHD_LOG_TRACE(unique_id(), "Initializing front-panel GPIO control...")
    _fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(
        _get_ctrl(0), regs::sr_addr(regs::FP_GPIO), regs::rb_addr(regs::RB_FP_GPIO));
}

void e3xx_radio_ctrl_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree, const size_t chan_idx)
{
    const fs_path tx_fe_path = fs_path("tx_frontends") / chan_idx;
    const fs_path rx_fe_path = fs_path("rx_frontends") / chan_idx;
    UHD_LOG_TRACE(unique_id(),
        "Adding non-RFNoC block properties for channel "
            << chan_idx << " to prop tree path " << tx_fe_path << " and " << rx_fe_path);
    // TX Standard attributes
    subtree->create<std::string>(tx_fe_path / "name").set(str(boost::format("E3xx")));
    subtree->create<std::string>(tx_fe_path / "connection").set("IQ");
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name").set(str(boost::format("E3xx")));
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
        .set(meta_range_t(AD9361_TX_MIN_FREQ, AD9361_TX_MAX_FREQ, 1.0))
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
        .set(meta_range_t(AD9361_RX_MIN_FREQ, AD9361_RX_MAX_FREQ, 1.0))
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set(AD9361_TX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_tx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return this->get_tx_bandwidth(chan_idx); });
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(AD9361_TX_MIN_BANDWIDTH, AD9361_TX_MAX_BANDWIDTH))
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        });
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set(AD9361_RX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw) {
            return this->set_rx_bandwidth(bw, chan_idx);
        });
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(AD9361_RX_MIN_BANDWIDTH, AD9361_RX_MAX_BANDWIDTH))
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
                [this, chan_idx]() { return radio_ctrl_impl::get_tx_gain(chan_idx); });
        subtree->create<meta_range_t>(tx_fe_path / "gains" / tx_gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([this]() {
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
                [this, chan_idx]() { return radio_ctrl_impl::get_rx_gain(chan_idx); });

        subtree->create<meta_range_t>(rx_fe_path / "gains" / rx_gain_name / "range")
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update gain range!");
            })
            .set_publisher([this]() {
                return meta_range_t(
                    AD9361_MIN_RX_GAIN, AD9361_MAX_RX_GAIN, AD9361_RX_GAIN_STEP);
            });
    }

    // TX LO lock sensor //////////////////////////////////////////////////////
    // Note: The AD9361 LO lock sensors are generated programmatically in
    // set_rpc_client(). The actual lo_locked publisher is also set there.
    subtree->create<sensor_value_t>(tx_fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false, "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this]() {
            return sensor_value_t(
                "all_los", this->get_lo_lock_status(TX_DIRECTION), "locked", "unlocked");
        });
    // RX LO lock sensor (see not on TX LO lock sensor)
    subtree->create<sensor_value_t>(rx_fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false, "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this]() {
            return sensor_value_t(
                "all_los", this->get_lo_lock_status(RX_DIRECTION), "locked", "unlocked");
        });
}

void e3xx_radio_ctrl_impl::_init_prop_tree()
{
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    for (size_t chan_idx = 0; chan_idx < E3XX_NUM_CHANS; chan_idx++) {
        this->_init_frontend_subtree(_tree->subtree(fe_base), chan_idx);
    }

    _tree->create<eeprom_map_t>(_root_path / "eeprom").set(eeprom_map_t());

    _tree->create<int>("rx_codecs" / _radio_slot / "gains");
    _tree->create<int>("tx_codecs" / _radio_slot / "gains");
    _tree->create<std::string>("rx_codecs" / _radio_slot / "name").set("AD9361 Dual ADC");
    _tree->create<std::string>("tx_codecs" / _radio_slot / "name").set("AD9361 Dual DAC");

    if (not _tree->exists("tick_rate")) {
        _tree->create<double>("tick_rate")
            .set_coercer([this](double tick_rate) { return this->set_rate(tick_rate); })
            .set_publisher([this]() { return this->get_rate(); });
    } else {
        UHD_LOG_WARNING(unique_id(), "Cannot set tick_rate again");
    }

    // *****FP_GPIO************************
    const fs_path ext_gpio_path = fs_path("gpio") / _fp_gpio_bank_name;
    for (const auto& attr : usrp::gpio_atr::gpio_attr_map) {
        if (not _tree->exists(ext_gpio_path / attr.second)) {
            switch (attr.first) {
                case usrp::gpio_atr::GPIO_SRC:
                    // This is not really the place to configure the source
                    // setting of the GPIO, don't have a better place to put this.
                    // Note: In UHD 4.0, this will move to the mb_controller
                    // object.
                    _tree->create<std::vector<std::string>>(ext_gpio_path / attr.second)
                        .set(std::vector<std::string>(
                            32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                        .add_coerced_subscriber(
                            [this, attr](const std::vector<std::string> str_val) {
                                uint32_t radio_src_value = 0;
                                uint32_t master_value    = 0;
                                for (size_t i = 0; i < str_val.size(); i++) {
                                    if (str_val[i] == "PS") {
                                        master_value += 1 << i;
                                        ;
                                    } else {
                                        auto port_num =
                                            _extract_port_number(str_val[i], _tree);
                                        radio_src_value =
                                            (1 << (2 * i)) * port_num + radio_src_value;
                                    }
                                }
                                _rpcc->notify_with_token(
                                    "set_fp_gpio_master", master_value);
                                _rpcc->notify_with_token(
                                    "set_fp_gpio_radio_src", radio_src_value);
                            });
                    break;
                case usrp::gpio_atr::GPIO_CTRL:
                case usrp::gpio_atr::GPIO_DDR:
                    _tree->create<std::vector<std::string>>(ext_gpio_path / attr.second)
                        .set(std::vector<std::string>(
                            32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                        .add_coerced_subscriber(
                            [this, attr](const std::vector<std::string> str_val) {
                                uint32_t val = 0;
                                for (size_t i = 0; i < str_val.size(); i++) {
                                    val += usrp::gpio_atr::gpio_attr_value_pair
                                               .at(attr.second)
                                               .at(str_val[i])
                                           << i;
                                }
                                _fp_gpio->set_gpio_attr(attr.first, val);
                            });
                    break;
                case usrp::gpio_atr::GPIO_READBACK: {
                    _tree->create<uint32_t>(ext_gpio_path / attr.second)
                        .set_publisher([this]() { return _fp_gpio->read_gpio(); });
                } break;
                default:
                    _tree->create<uint32_t>(ext_gpio_path / attr.second)
                        .set(0)
                        .add_coerced_subscriber([this, attr](const uint32_t val) {
                            _fp_gpio->set_gpio_attr(attr.first, val);
                        });
            }
        } else {
            switch (attr.first) {
                case usrp::gpio_atr::GPIO_SRC:
                    break;
                case usrp::gpio_atr::GPIO_CTRL:
                case usrp::gpio_atr::GPIO_DDR:
                    _tree->access<std::vector<std::string>>(ext_gpio_path / attr.second)
                        .set(std::vector<std::string>(
                            32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                        .add_coerced_subscriber(
                            [this, attr](const std::vector<std::string> str_val) {
                                uint32_t val = 0;
                                for (size_t i = 0; i < str_val.size(); i++) {
                                    val += usrp::gpio_atr::gpio_attr_value_pair
                                               .at(attr.second)
                                               .at(str_val[i])
                                           << i;
                                }
                                _fp_gpio->set_gpio_attr(attr.first, val);
                            });
                    break;
                case usrp::gpio_atr::GPIO_READBACK:
                    break;
                default:
                    _tree->access<uint32_t>(ext_gpio_path / attr.second)
                        .set(0)
                        .add_coerced_subscriber([this, attr](const uint32_t val) {
                            _fp_gpio->set_gpio_attr(attr.first, val);
                        });
            }
        }
    }
}

void e3xx_radio_ctrl_impl::_init_codec()
{
    for (size_t chan = 0; chan < _get_num_radios(); chan++) {
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

void e3xx_radio_ctrl_impl::_init_mpm_sensors(const direction_t dir, const size_t chan_idx)
{
    const std::string trx = (dir == RX_DIRECTION) ? "RX" : "TX";
    const fs_path fe_path = fs_path("dboards") / _radio_slot
                            / (dir == RX_DIRECTION ? "rx_frontends" : "tx_frontends")
                            / chan_idx;
    auto sensor_list = _rpcc->request_with_token<std::vector<std::string>>(
        this->_rpc_prefix + "get_sensors", trx);
    UHD_LOG_TRACE(unique_id(),
        "Chan " << chan_idx << ": Found " << sensor_list.size() << " " << trx
                << " sensors.");
    for (const auto& sensor_name : sensor_list) {
        UHD_LOG_TRACE(unique_id(), "Adding " << trx << " sensor " << sensor_name);
        _tree->create<sensor_value_t>(fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, trx, sensor_name, chan_idx]() {
                return sensor_value_t(
                    this->_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
                        this->_rpc_prefix + "get_sensor", trx, sensor_name, chan_idx));
            });
    }
}
