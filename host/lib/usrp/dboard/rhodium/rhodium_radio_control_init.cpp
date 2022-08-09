//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_constants.hpp"
#include "rhodium_radio_control.hpp"
#include <uhd/types/eeprom.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
enum slave_select_t {
    SEN_CPLD    = 8,
    SEN_TX_LO   = 1,
    SEN_RX_LO   = 2,
    SEN_LO_DIST = 4 /* Unused */
};

constexpr double RHODIUM_DEFAULT_FREQ = 2.5e9; // Hz
// An invalid default index ensures that set gain will apply settings
// the first time it is called
constexpr double RHODIUM_DEFAULT_INVALID_GAIN = -1; // gain index
constexpr double RHODIUM_DEFAULT_GAIN         = 0; // gain index
constexpr double RHODIUM_DEFAULT_LO_GAIN      = 30; // gain index
constexpr char RHODIUM_DEFAULT_RX_ANTENNA[]   = "RX2";
constexpr char RHODIUM_DEFAULT_TX_ANTENNA[]   = "TX/RX";
constexpr auto RHODIUM_DEFAULT_MASH_ORDER     = lmx2592_iface::mash_order_t::THIRD;
constexpr double DEFAULT_IDENTIFY_DURATION    = 5.0; // seconds


//! Returns the SPI config used by the CPLD
spi_config_t _get_cpld_spi_config()
{
    spi_config_t spi_config;
    spi_config.use_custom_divider = true;
    spi_config.divider            = 10;
    spi_config.mosi_edge          = spi_config_t::EDGE_RISE;
    spi_config.miso_edge          = spi_config_t::EDGE_FALL;

    return spi_config;
}

//! Returns the SPI config used by the TX LO
spi_config_t _get_tx_lo_spi_config()
{
    spi_config_t spi_config;
    spi_config.use_custom_divider = true;
    spi_config.divider            = 10;
    spi_config.mosi_edge          = spi_config_t::EDGE_RISE;
    spi_config.miso_edge          = spi_config_t::EDGE_FALL;

    return spi_config;
}

//! Returns the SPI config used by the RX LO
spi_config_t _get_rx_lo_spi_config()
{
    spi_config_t spi_config;
    spi_config.use_custom_divider = true;
    spi_config.divider            = 10;
    spi_config.mosi_edge          = spi_config_t::EDGE_RISE;
    spi_config.miso_edge          = spi_config_t::EDGE_FALL;

    return spi_config;
}

std::function<void(uint32_t)> _generate_write_spi(
    uhd::spi_iface::sptr spi, slave_select_t slave, spi_config_t config)
{
    return [spi, slave, config](const uint32_t transaction) {
        spi->write_spi(slave, config, transaction, 24);
    };
}

std::function<uint32_t(uint32_t)> _generate_read_spi(
    uhd::spi_iface::sptr spi, slave_select_t slave, spi_config_t config)
{
    return [spi, slave, config](const uint32_t transaction) {
        return spi->read_spi(slave, config, transaction, 24);
    };
}
} // namespace

void rhodium_radio_control_impl::_init_defaults()
{
    RFNOC_LOG_TRACE("Initializing defaults...");
    const size_t num_rx_chans = get_num_output_ports();
    const size_t num_tx_chans = get_num_input_ports();
    UHD_ASSERT_THROW(num_tx_chans == RHODIUM_NUM_CHANS);
    UHD_ASSERT_THROW(num_rx_chans == RHODIUM_NUM_CHANS);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_control_impl::set_rx_frequency(RHODIUM_DEFAULT_FREQ, chan);
        radio_control_impl::set_rx_gain(RHODIUM_DEFAULT_INVALID_GAIN, chan);
        radio_control_impl::set_rx_antenna(RHODIUM_DEFAULT_RX_ANTENNA, chan);
        radio_control_impl::set_rx_bandwidth(RHODIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_control_impl::set_tx_frequency(RHODIUM_DEFAULT_FREQ, chan);
        radio_control_impl::set_tx_gain(RHODIUM_DEFAULT_INVALID_GAIN, chan);
        radio_control_impl::set_tx_antenna(RHODIUM_DEFAULT_TX_ANTENNA, chan);
        radio_control_impl::set_tx_bandwidth(RHODIUM_DEFAULT_BANDWIDTH, chan);
    }

    register_property(&_spur_dodging_mode);
    register_property(&_spur_dodging_threshold);
    register_property(&_highband_spur_reduction_mode);

    // Update configurable block arguments from the device arguments provided
    const auto block_args = get_block_args();
    if (block_args.has_key(SPUR_DODGING_PROP_NAME)) {
        _spur_dodging_mode.set(block_args.get(SPUR_DODGING_PROP_NAME));
    }
    if (block_args.has_key(SPUR_DODGING_THRESHOLD_PROP_NAME)) {
        _spur_dodging_threshold.set(block_args.cast<double>(
            SPUR_DODGING_THRESHOLD_PROP_NAME, RHODIUM_DEFAULT_SPUR_DOGING_THRESHOLD));
    }
    if (block_args.has_key(HIGHBAND_SPUR_REDUCTION_PROP_NAME)) {
        _highband_spur_reduction_mode.set(
            block_args.get(HIGHBAND_SPUR_REDUCTION_PROP_NAME));
    }
}

void rhodium_radio_control_impl::_init_peripherals()
{
    RFNOC_LOG_TRACE("Initializing SPI core...");
    _spi = spi_core_3000::make(
        [this](uint32_t addr, uint32_t data) {
            regs().poke32(addr, data, get_command_time(0));
        },
        [this](uint32_t addr) { return regs().peek32(addr, get_command_time(0)); },
        n320_regs::SR_SPI,
        8,
        n320_regs::RB_SPI);
    _wb_iface = RFNOC_MAKE_WB_IFACE(0, 0);

    RFNOC_LOG_TRACE("Initializing CPLD...");
    _cpld = std::make_shared<rhodium_cpld_ctrl>(
        _generate_write_spi(this->_spi, SEN_CPLD, _get_cpld_spi_config()),
        _generate_read_spi(this->_spi, SEN_CPLD, _get_cpld_spi_config()));

    RFNOC_LOG_TRACE("Initializing TX frontend DSP core...")
    _tx_fe_core = tx_frontend_core_200::make(
        _wb_iface, n320_regs::SR_TX_FE_BASE, n320_regs::PERIPH_REG_OFFSET);
    _tx_fe_core->set_dc_offset(tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
    _tx_fe_core->set_iq_balance(tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
    _tx_fe_core->populate_subtree(get_tree()->subtree(FE_PATH / "tx_fe_corrections" / 0));

    RFNOC_LOG_TRACE("Initializing RX frontend DSP core...")
    _rx_fe_core = rx_frontend_core_3000::make(
        _wb_iface, n320_regs::SR_RX_FE_BASE, n320_regs::PERIPH_REG_OFFSET);
    _rx_fe_core->set_adc_rate(_master_clock_rate);
    _rx_fe_core->set_dc_offset(rx_frontend_core_3000::DEFAULT_DC_OFFSET_VALUE);
    _rx_fe_core->set_dc_offset_auto(rx_frontend_core_3000::DEFAULT_DC_OFFSET_ENABLE);
    _rx_fe_core->set_iq_balance(rx_frontend_core_3000::DEFAULT_IQ_BALANCE_VALUE);
    _rx_fe_core->populate_subtree(get_tree()->subtree(FE_PATH / "rx_fe_corrections" / 0));

    RFNOC_LOG_TRACE("Writing initial gain values...");
    set_tx_gain(RHODIUM_DEFAULT_GAIN, 0);
    set_tx_lo_gain(RHODIUM_DEFAULT_LO_GAIN, RHODIUM_LO1, 0);
    set_rx_gain(RHODIUM_DEFAULT_GAIN, 0);
    set_rx_lo_gain(RHODIUM_DEFAULT_LO_GAIN, RHODIUM_LO1, 0);

    RFNOC_LOG_TRACE("Initializing TX LO...");
    _tx_lo = lmx2592_iface::make(
        _generate_write_spi(this->_spi, SEN_TX_LO, _get_tx_lo_spi_config()),
        _generate_read_spi(this->_spi, SEN_TX_LO, _get_tx_lo_spi_config()));

    RFNOC_LOG_TRACE("Writing initial TX LO state...");
    _tx_lo->set_reference_frequency(RHODIUM_LO1_REF_FREQ);
    _tx_lo->set_mash_order(RHODIUM_DEFAULT_MASH_ORDER);

    RFNOC_LOG_TRACE("Initializing RX LO...");
    _rx_lo = lmx2592_iface::make(
        _generate_write_spi(this->_spi, SEN_RX_LO, _get_rx_lo_spi_config()),
        _generate_read_spi(this->_spi, SEN_RX_LO, _get_rx_lo_spi_config()));

    RFNOC_LOG_TRACE("Writing initial RX LO state...");
    _rx_lo->set_reference_frequency(RHODIUM_LO1_REF_FREQ);
    _rx_lo->set_mash_order(RHODIUM_DEFAULT_MASH_ORDER);

    RFNOC_LOG_TRACE("Initializing GPIOs...");
    // DB GPIOs
    _gpio = usrp::gpio_atr::gpio_atr_3000::make(_wb_iface,
        gpio_atr::gpio_atr_offsets::make_default(
            n320_regs::SR_DB_GPIO,
            n320_regs::RB_DB_GPIO,
            n320_regs::PERIPH_REG_OFFSET));
    _gpio->set_atr_mode(usrp::gpio_atr::MODE_ATR, // Enable ATR mode for Rhodium bits
        RHODIUM_GPIO_MASK);
    _gpio->set_atr_mode(usrp::gpio_atr::MODE_GPIO, // Disable ATR mode for unused bits
        ~RHODIUM_GPIO_MASK);
    _gpio->set_gpio_ddr(usrp::gpio_atr::DDR_OUTPUT, // Make all GPIOs outputs
        usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    _fp_gpio = gpio_atr::gpio_atr_3000::make(_wb_iface,
        gpio_atr::gpio_atr_offsets::make_default(
            n320_regs::SR_FP_GPIO,
            n320_regs::RB_FP_GPIO,
            n320_regs::PERIPH_REG_OFFSET));

    RFNOC_LOG_TRACE("Set initial ATR values...");
    _update_atr(RHODIUM_DEFAULT_TX_ANTENNA, TX_DIRECTION);
    _update_atr(RHODIUM_DEFAULT_RX_ANTENNA, RX_DIRECTION);

    // Updating the TX frequency path may include an update to SW10, which is
    // GPIO controlled, so this must follow CPLD and GPIO initialization
    RFNOC_LOG_TRACE("Writing initial switch values...");
    _update_tx_freq_switches(RHODIUM_DEFAULT_FREQ);
    _update_rx_freq_switches(RHODIUM_DEFAULT_FREQ);

    // Antenna setting requires both CPLD and GPIO control
    RFNOC_LOG_TRACE("Setting initial antenna settings");
    _update_tx_output_switches(RHODIUM_DEFAULT_TX_ANTENNA);
    _update_rx_input_switches(RHODIUM_DEFAULT_RX_ANTENNA);

    RFNOC_LOG_TRACE("Checking for existence of LO Distribution board");
    _lo_dist_present =
        _rpcc->request_with_token<bool>(_rpc_prefix + "is_lo_dist_present");
    RFNOC_LOG_DEBUG(
        "LO distribution board is" << (_lo_dist_present ? "" : " NOT") << " present");

    RFNOC_LOG_TRACE("Reading EEPROM content...");
    const size_t db_idx = get_block_id().get_block_count();
    _db_eeprom = this->_rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", db_idx);
}

// Reminder: The property must not own any properties, it can only interact with
// the API of this block!
void rhodium_radio_control_impl::_init_frontend_subtree(uhd::property_tree::sptr subtree)
{
    const fs_path tx_fe_path = fs_path("tx_frontends") / 0;
    const fs_path rx_fe_path = fs_path("rx_frontends") / 0;
    RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel 0"
                    << " to prop tree path " << tx_fe_path << " and " << rx_fe_path);
    // TX Standard attributes
    subtree->create<std::string>(tx_fe_path / "name").set(RHODIUM_FE_NAME);
    subtree->create<std::string>(tx_fe_path / "connection")
        .add_coerced_subscriber(
            [this](const std::string& conn) { this->_set_tx_fe_connection(conn); })
        .set_publisher([this]() { return this->_get_tx_fe_connection(); });
    subtree->create<device_addr_t>(tx_fe_path / "tune_args")
        .set(device_addr_t())
        .add_coerced_subscriber(
            [this](const device_addr_t& args) { set_tx_tune_args(args, 0); })
        .set_publisher([this]() { return _tune_args.at(uhd::TX_DIRECTION); });
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name").set(RHODIUM_FE_NAME);
    subtree->create<std::string>(rx_fe_path / "connection")
        .add_coerced_subscriber(
            [this](const std::string& conn) { this->_set_rx_fe_connection(conn); })
        .set_publisher([this]() { return this->_get_rx_fe_connection(); });
    subtree->create<device_addr_t>(rx_fe_path / "tune_args")
        .set(device_addr_t())
        .add_coerced_subscriber(
            [this](const device_addr_t& args) { set_rx_tune_args(args, 0); })
        .set_publisher([this]() { return _tune_args.at(uhd::RX_DIRECTION); });
    ;
    // TX Antenna
    subtree->create<std::string>(tx_fe_path / "antenna" / "value")
        .add_coerced_subscriber(
            [this](const std::string& ant) { this->set_tx_antenna(ant, 0); })
        .set_publisher([this]() { return this->get_tx_antenna(0); });
    subtree->create<std::vector<std::string>>(tx_fe_path / "antenna" / "options")
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        })
        .set_publisher([this]() { return get_tx_antennas(0); });
    // RX Antenna
    subtree->create<std::string>(rx_fe_path / "antenna" / "value")
        .add_coerced_subscriber(
            [this](const std::string& ant) { this->set_rx_antenna(ant, 0); })
        .set_publisher([this]() { return this->get_rx_antenna(0); });
    subtree->create<std::vector<std::string>>(rx_fe_path / "antenna" / "options")
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        })
        .set_publisher([this]() { return get_rx_antennas(0); });
    // TX frequency
    subtree->create<double>(tx_fe_path / "freq" / "value")
        .set_coercer(
            [this](const double freq) { return this->set_tx_frequency(freq, 0); })
        .set_publisher([this]() { return this->get_tx_frequency(0); });
    subtree->create<meta_range_t>(tx_fe_path / "freq" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        })
        .set_publisher([this]() { return get_tx_frequency_range(0); });
    // RX frequency
    subtree->create<double>(rx_fe_path / "freq" / "value")
        .set_coercer(
            [this](const double freq) { return this->set_rx_frequency(freq, 0); })
        .set_publisher([this]() { return this->get_rx_frequency(0); });
    subtree->create<meta_range_t>(rx_fe_path / "freq" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        })
        .set_publisher([this]() { return get_rx_frequency_range(0); });
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set_coercer([this](const double bw) { return this->set_tx_bandwidth(bw, 0); })
        .set_publisher([this]() { return this->get_tx_bandwidth(0); });
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        })
        .set_publisher([this]() { return get_tx_bandwidth_range(0); });
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set_coercer([this](const double bw) { return this->set_rx_bandwidth(bw, 0); })
        .set_publisher([this]() { return this->get_rx_bandwidth(0); });
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update bandwidth range!");
        })
        .set_publisher([this]() { return get_rx_bandwidth_range(0); });
    // TX gains
    subtree->create<double>(tx_fe_path / "gains" / "all" / "value")
        .set_coercer([this](const double gain) { return this->set_tx_gain(gain, 0); })
        .set_publisher([this]() { return radio_control_impl::get_tx_gain(0); });
    subtree->create<meta_range_t>(tx_fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update gain range!");
        })
        .set_publisher([this]() { return get_tx_gain_range(0); });
    // RX gains
    subtree->create<double>(rx_fe_path / "gains" / "all" / "value")
        .set_coercer([this](const double gain) { return this->set_rx_gain(gain, 0); })
        .set_publisher([this]() { return radio_control_impl::get_rx_gain(0); });
    subtree->create<meta_range_t>(rx_fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update gain range!");
        })
        .set_publisher([this]() { return get_rx_gain_range(0); });

    // LO Specific
    // RX LO
    // RX LO1 Frequency
    subtree->create<double>(rx_fe_path / "los" / RHODIUM_LO1 / "freq/value")
        .set_publisher([this]() { return this->get_rx_lo_freq(RHODIUM_LO1, 0); })
        .set_coercer([this](const double freq) {
            return this->set_rx_lo_freq(freq, RHODIUM_LO1, 0);
        });
    subtree->create<meta_range_t>(rx_fe_path / "los" / RHODIUM_LO1 / "freq/range")
        .set_publisher([this]() { return this->get_rx_lo_freq_range(RHODIUM_LO1, 0); });
    // RX LO1 Source
    subtree
        ->create<std::vector<std::string>>(
            rx_fe_path / "los" / RHODIUM_LO1 / "source/options")
        .set_publisher([this]() { return this->get_rx_lo_sources(RHODIUM_LO1, 0); });
    subtree->create<std::string>(rx_fe_path / "los" / RHODIUM_LO1 / "source/value")
        .add_coerced_subscriber([this](const std::string& src) {
            this->set_rx_lo_source(src, RHODIUM_LO1, 0);
        })
        .set_publisher([this]() { return this->get_rx_lo_source(RHODIUM_LO1, 0); });
    // RX LO1 Export
    subtree->create<bool>(rx_fe_path / "los" / RHODIUM_LO1 / "export")
        .add_coerced_subscriber([this](bool enabled) {
            this->set_rx_lo_export_enabled(enabled, RHODIUM_LO1, 0);
        });
    // RX LO1 Gain
    subtree
        ->create<double>(
            rx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_GAIN / "value")
        .set_publisher([this]() { return this->get_rx_lo_gain(RHODIUM_LO1, 0); })
        .set_coercer([this](const double gain) {
            return this->set_rx_lo_gain(gain, RHODIUM_LO1, 0);
        });
    subtree
        ->create<meta_range_t>(
            rx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_GAIN / "range")
        .set_publisher([]() { return rhodium_radio_control_impl::_get_lo_gain_range(); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update LO gain range!");
        });
    // RX LO1 Output Power
    subtree
        ->create<double>(
            rx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_POWER / "value")
        .set_publisher([this]() { return this->get_rx_lo_power(RHODIUM_LO1, 0); })
        .set_coercer([this](const double gain) {
            return this->set_rx_lo_power(gain, RHODIUM_LO1, 0);
        });
    subtree
        ->create<meta_range_t>(
            rx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_POWER / "range")
        .set_publisher([]() { return rhodium_radio_control_impl::_get_lo_power_range(); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update LO output power range!");
        });
    // RX LO2 Frequency
    subtree->create<double>(rx_fe_path / "los" / RHODIUM_LO2 / "freq/value")
        .set_publisher([this]() { return this->get_rx_lo_freq(RHODIUM_LO2, 0); })
        .set_coercer(
            [this](double freq) { return this->set_rx_lo_freq(freq, RHODIUM_LO2, 0); });
    subtree->create<meta_range_t>(rx_fe_path / "los" / RHODIUM_LO2 / "freq/range")
        .set_publisher([this]() { return this->get_rx_lo_freq_range(RHODIUM_LO2, 0); });
    // RX LO2 Source
    subtree
        ->create<std::vector<std::string>>(
            rx_fe_path / "los" / RHODIUM_LO2 / "source/options")
        .set_publisher([this]() { return this->get_rx_lo_sources(RHODIUM_LO2, 0); });
    subtree->create<std::string>(rx_fe_path / "los" / RHODIUM_LO2 / "source/value")
        .add_coerced_subscriber(
            [this](std::string src) { this->set_rx_lo_source(src, RHODIUM_LO2, 0); })
        .set_publisher([this]() { return this->get_rx_lo_source(RHODIUM_LO2, 0); });
    // RX LO2 Export
    subtree->create<bool>(rx_fe_path / "los" / RHODIUM_LO2 / "export")
        .add_coerced_subscriber([this](bool enabled) {
            this->set_rx_lo_export_enabled(enabled, RHODIUM_LO2, 0);
        });
    // RX ALL LOs
    subtree->create<std::string>(rx_fe_path / "los" / ALL_LOS / "source/value")
        .add_coerced_subscriber(
            [this](std::string src) { this->set_rx_lo_source(src, ALL_LOS, 0); })
        .set_publisher([this]() { return this->get_rx_lo_source(ALL_LOS, 0); });
    subtree
        ->create<std::vector<std::string>>(
            rx_fe_path / "los" / ALL_LOS / "source/options")
        .set_publisher([this]() { return this->get_rx_lo_sources(ALL_LOS, 0); });
    subtree->create<bool>(rx_fe_path / "los" / ALL_LOS / "export")
        .add_coerced_subscriber(
            [this](bool enabled) { this->set_rx_lo_export_enabled(enabled, ALL_LOS, 0); })
        .set_publisher([this]() { return this->get_rx_lo_export_enabled(ALL_LOS, 0); });
    // TX LO
    // TX LO1 Frequency
    subtree->create<double>(tx_fe_path / "los" / RHODIUM_LO1 / "freq/value ")
        .set_publisher([this]() { return this->get_tx_lo_freq(RHODIUM_LO1, 0); })
        .set_coercer(
            [this](double freq) { return this->set_tx_lo_freq(freq, RHODIUM_LO1, 0); });
    subtree->create<meta_range_t>(tx_fe_path / "los" / RHODIUM_LO1 / "freq/range")
        .set_publisher([this]() { return this->get_rx_lo_freq_range(RHODIUM_LO1, 0); });
    // TX LO1 Source
    subtree
        ->create<std::vector<std::string>>(
            tx_fe_path / "los" / RHODIUM_LO1 / "source/options")
        .set_publisher([this]() { return this->get_tx_lo_sources(RHODIUM_LO1, 0); });
    subtree->create<std::string>(tx_fe_path / "los" / RHODIUM_LO1 / "source/value")
        .add_coerced_subscriber(
            [this](std::string src) { this->set_tx_lo_source(src, RHODIUM_LO1, 0); })
        .set_publisher([this]() { return this->get_tx_lo_source(RHODIUM_LO1, 0); });
    // TX LO1 Export
    subtree->create<bool>(tx_fe_path / "los" / RHODIUM_LO1 / "export")
        .add_coerced_subscriber([this](bool enabled) {
            this->set_tx_lo_export_enabled(enabled, RHODIUM_LO1, 0);
        });
    // TX LO1 Gain
    subtree
        ->create<double>(
            tx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_GAIN / "value")
        .set_publisher([this]() { return this->get_tx_lo_gain(RHODIUM_LO1, 0); })
        .set_coercer([this](const double gain) {
            return this->set_tx_lo_gain(gain, RHODIUM_LO1, 0);
        });
    subtree
        ->create<meta_range_t>(
            tx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_GAIN / "range")
        .set_publisher([]() { return rhodium_radio_control_impl::_get_lo_gain_range(); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update LO gain range!");
        });
    // TX LO1 Output Power
    subtree
        ->create<double>(
            tx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_POWER / "value")
        .set_publisher([this]() { return this->get_tx_lo_power(RHODIUM_LO1, 0); })
        .set_coercer([this](const double gain) {
            return this->set_tx_lo_power(gain, RHODIUM_LO1, 0);
        });
    subtree
        ->create<meta_range_t>(
            tx_fe_path / "los" / RHODIUM_LO1 / "gains" / RHODIUM_LO_POWER / "range")
        .set_publisher([]() { return rhodium_radio_control_impl::_get_lo_power_range(); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update LO output power range!");
        });
    // TX LO2 Frequency
    subtree->create<double>(tx_fe_path / "los" / RHODIUM_LO2 / "freq/value")
        .set_publisher([this]() { return this->get_tx_lo_freq(RHODIUM_LO2, 0); })
        .set_coercer(
            [this](double freq) { return this->set_tx_lo_freq(freq, RHODIUM_LO2, 0); });
    subtree->create<meta_range_t>(tx_fe_path / "los" / RHODIUM_LO2 / "freq/range")
        .set_publisher([this]() { return this->get_tx_lo_freq_range(RHODIUM_LO2, 0); });
    // TX LO2 Source
    subtree
        ->create<std::vector<std::string>>(
            tx_fe_path / "los" / RHODIUM_LO2 / "source/options")
        .set_publisher([this]() { return this->get_tx_lo_sources(RHODIUM_LO2, 0); });
    subtree->create<std::string>(tx_fe_path / "los" / RHODIUM_LO2 / "source/value")
        .add_coerced_subscriber(
            [this](std::string src) { this->set_tx_lo_source(src, RHODIUM_LO2, 0); })
        .set_publisher([this]() { return this->get_tx_lo_source(RHODIUM_LO2, 0); });
    // TX LO2 Export
    subtree->create<bool>(tx_fe_path / "los" / RHODIUM_LO2 / "export")
        .add_coerced_subscriber([this](bool enabled) {
            this->set_tx_lo_export_enabled(enabled, RHODIUM_LO2, 0);
        });
    // TX ALL LOs
    subtree->create<std::string>(tx_fe_path / "los" / ALL_LOS / "source/value")
        .add_coerced_subscriber(
            [this](std::string src) { this->set_tx_lo_source(src, ALL_LOS, 0); })
        .set_publisher([this]() { return this->get_tx_lo_source(ALL_LOS, 0); });
    subtree
        ->create<std::vector<std::string>>(
            tx_fe_path / "los" / ALL_LOS / "source/options")
        .set_publisher([this]() { return this->get_tx_lo_sources(ALL_LOS, 0); });
    subtree->create<bool>(tx_fe_path / "los" / ALL_LOS / "export")
        .add_coerced_subscriber(
            [this](bool enabled) { this->set_tx_lo_export_enabled(enabled, ALL_LOS, 0); })
        .set_publisher([this]() { return this->get_tx_lo_export_enabled(ALL_LOS, 0); });

    // LO Distribution Output Ports
    if (_lo_dist_present) {
        for (const auto& port : LO_OUTPUT_PORT_NAMES) {
            subtree
                ->create<bool>(tx_fe_path / "los" / RHODIUM_LO1 / "lo_distribution" / port
                               / "export")
                .add_coerced_subscriber([this, port](bool enabled) {
                    this->set_tx_lo_output_enabled(enabled, port, 0);
                })
                .set_publisher(
                    [this, port]() { return this->get_tx_lo_output_enabled(port, 0); });
            subtree
                ->create<bool>(rx_fe_path / "los" / RHODIUM_LO1 / "lo_distribution" / port
                               / "export")
                .add_coerced_subscriber([this, port](bool enabled) {
                    this->set_rx_lo_output_enabled(enabled, port, 0);
                })
                .set_publisher(
                    [this, port]() { return this->get_rx_lo_output_enabled(port, 0); });
        }
    }

    // Sensors
    auto rx_sensor_names = get_rx_sensor_names(0);
    for (const auto& sensor_name : rx_sensor_names) {
        RFNOC_LOG_TRACE("Adding RX sensor " << sensor_name);
        get_tree()
            ->create<sensor_value_t>(rx_fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher(
                [this, sensor_name]() { return get_rx_sensor(sensor_name, 0); });
    }
    auto tx_sensor_names = get_tx_sensor_names(0);
    for (const auto& sensor_name : tx_sensor_names) {
        RFNOC_LOG_TRACE("Adding TX sensor " << sensor_name);
        get_tree()
            ->create<sensor_value_t>(tx_fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher(
                [this, sensor_name]() { return get_tx_sensor(sensor_name, 0); });
    }
}

void rhodium_radio_control_impl::_init_prop_tree()
{
    this->_init_frontend_subtree(get_tree()->subtree(DB_PATH));
    get_tree()->create<std::string>(fs_path("rx_codecs") / "name").set("ad9695-625");
    get_tree()->create<std::string>(fs_path("tx_codecs") / "name").set("dac37j82");
}

void rhodium_radio_control_impl::_init_mpm()
{
    auto block_args = get_block_args();
    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration          = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = DEFAULT_IDENTIFY_DURATION;
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
        throw uhd::runtime_error(
            std::string("Master clock rate mismatch. Device returns ")
            + std::to_string(_master_clock_rate)
            + " MHz, "
              "but should have been "
            + std::to_string(
                  block_args.cast<double>("master_clock_rate", _master_clock_rate))
            + " MHz.");
    }
    RFNOC_LOG_DEBUG("Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    set_tick_rate(_master_clock_rate);
    _n3xx_timekeeper->update_tick_rate(_master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);

    // Unlike N310, N320 does not have any MPM sensors.
}
