//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_constants.hpp"
#include "rhodium_radio_ctrl_impl.hpp"
#include <uhd/transport/chdr.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
    enum slave_select_t {
        SEN_CPLD = 8,
        SEN_TX_LO = 1,
        SEN_RX_LO = 2,
        SEN_LO_DIST = 4 /* Unused */
    };

    constexpr uint32_t TX_FE_BASE = 224;
    constexpr uint32_t RX_FE_BASE = 232;

    constexpr double RHODIUM_DEFAULT_FREQ         = 2.5e9; // Hz
    // An invalid default index ensures that set gain will apply settings
    // the first time it is called
    constexpr double RHODIUM_DEFAULT_INVALID_GAIN = -1; // gain index
    constexpr double RHODIUM_DEFAULT_GAIN         = 0;  // gain index
    constexpr double RHODIUM_DEFAULT_LO_GAIN      = 30; // gain index
    constexpr char   RHODIUM_DEFAULT_RX_ANTENNA[] = "RX2";
    constexpr char   RHODIUM_DEFAULT_TX_ANTENNA[] = "TX/RX";
    constexpr double RHODIUM_DEFAULT_BANDWIDTH    = 250e6; // Hz
    constexpr auto   RHODIUM_DEFAULT_MASH_ORDER   = lmx2592_iface::mash_order_t::THIRD;

    //! Rhodium gain profile options
    const std::vector<std::string> RHODIUM_GP_OPTIONS = {
        "default"
    };

    //! Returns the SPI config used by the CPLD
    spi_config_t _get_cpld_spi_config() {
        spi_config_t spi_config;
        spi_config.use_custom_divider = true;
        spi_config.divider = 10;
        spi_config.mosi_edge = spi_config_t::EDGE_RISE;
        spi_config.miso_edge = spi_config_t::EDGE_FALL;

        return spi_config;
    }

    //! Returns the SPI config used by the TX LO
    spi_config_t _get_tx_lo_spi_config() {
        spi_config_t spi_config;
        spi_config.use_custom_divider = true;
        spi_config.divider = 10;
        spi_config.mosi_edge = spi_config_t::EDGE_RISE;
        spi_config.miso_edge = spi_config_t::EDGE_FALL;

        return spi_config;
    }

    //! Returns the SPI config used by the RX LO
    spi_config_t _get_rx_lo_spi_config() {
        spi_config_t spi_config;
        spi_config.use_custom_divider = true;
        spi_config.divider = 10;
        spi_config.mosi_edge = spi_config_t::EDGE_RISE;
        spi_config.miso_edge = spi_config_t::EDGE_FALL;

        return spi_config;
    }

    std::function<void(uint32_t)> _generate_write_spi(
        uhd::spi_iface::sptr spi,
        slave_select_t slave,
        spi_config_t config
    ) {
        return [spi, slave, config](const uint32_t transaction) {
            spi->write_spi(slave, config, transaction, 24);
        };
    }

    std::function<uint32_t(uint32_t)> _generate_read_spi(
        uhd::spi_iface::sptr spi,
        slave_select_t slave,
        spi_config_t config
    ) {
        return [spi, slave, config](const uint32_t transaction) {
            return spi->read_spi(slave, config, transaction, 24);
        };
    }

    //! Helper function to extract single value of port number.
    //
    // Each GPIO pins can be controlled by each radio output ports.
    // This function convert the format of attribute "Radio_N_M"
    // to a single value port number = N*number_of_port_per_radio + M

    uint32_t extract_port_number(
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
            throw uhd::runtime_error(
                str(boost::format("%s is an invalid GPIO source string.")
                    % radio_src_string));
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
}

void rhodium_radio_ctrl_impl::_init_defaults()
{
    UHD_LOG_TRACE(unique_id(), "Initializing defaults...");
    const size_t num_rx_chans = get_output_ports().size();
    const size_t num_tx_chans = get_input_ports().size();

    UHD_LOG_TRACE(unique_id(),
            "Num TX chans: " << num_tx_chans
            << " Num RX chans: " << num_rx_chans);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(RHODIUM_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(RHODIUM_DEFAULT_INVALID_GAIN, chan);
        radio_ctrl_impl::set_rx_antenna(RHODIUM_DEFAULT_RX_ANTENNA, chan);
        radio_ctrl_impl::set_rx_bandwidth(RHODIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_ctrl_impl::set_tx_frequency(RHODIUM_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_tx_gain(RHODIUM_DEFAULT_INVALID_GAIN, chan);
        radio_ctrl_impl::set_tx_antenna(RHODIUM_DEFAULT_TX_ANTENNA, chan);
        radio_ctrl_impl::set_tx_bandwidth(RHODIUM_DEFAULT_BANDWIDTH, chan);
    }

    /** Update default SPP (overwrites the default value from the XML file) **/
    const size_t max_bytes_header =
        uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t);
    const size_t default_spp =
        (_tree->access<size_t>("mtu/recv").get() - max_bytes_header)
        / (2 * sizeof(int16_t));
    UHD_LOG_DEBUG(unique_id(),
        "Setting default spp to " << default_spp);
    _tree->access<int>(get_arg_path("spp") / "value").set(default_spp);

    // Update configurable block arguments from the device arguments provided
    if (_block_args.has_key(SPUR_DODGING_ARG_NAME)) {
        _tree->access<std::string>(get_arg_path(SPUR_DODGING_ARG_NAME) / "value")
            .set(_block_args.get(SPUR_DODGING_ARG_NAME));
    }

    if (_block_args.has_key(SPUR_DODGING_THRESHOLD_ARG_NAME)) {
        _tree->access<double>(get_arg_path(SPUR_DODGING_THRESHOLD_ARG_NAME) / "value")
            .set(boost::lexical_cast<double>(_block_args.get(SPUR_DODGING_THRESHOLD_ARG_NAME)));
    }

    if (_block_args.has_key(HIGHBAND_SPUR_REDUCTION_ARG_NAME)) {
        _tree
            ->access<std::string>(
                get_arg_path(HIGHBAND_SPUR_REDUCTION_ARG_NAME) / "value")
            .set(_block_args.get(HIGHBAND_SPUR_REDUCTION_ARG_NAME));
    }
}

void rhodium_radio_ctrl_impl::_init_peripherals()
{
    UHD_LOG_TRACE(unique_id(), "Initializing peripherals...");

    UHD_LOG_TRACE(unique_id(), "Initializing SPI core...");
    _spi = spi_core_3000::make(_get_ctrl(0),
        regs::sr_addr(regs::SPI),
        regs::rb_addr(regs::RB_SPI)
    );

    UHD_LOG_TRACE(unique_id(), "Initializing CPLD...");
    _cpld = std::make_shared<rhodium_cpld_ctrl>(
        _generate_write_spi(this->_spi, SEN_CPLD, _get_cpld_spi_config()),
        _generate_read_spi(this->_spi, SEN_CPLD, _get_cpld_spi_config()));

    UHD_LOG_TRACE(unique_id(), "Initializing TX frontend DSP core...")
    _tx_fe_core = tx_frontend_core_200::make(_get_ctrl(0), regs::sr_addr(TX_FE_BASE));
    _tx_fe_core->set_dc_offset(tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
    _tx_fe_core->set_iq_balance(tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
    _tx_fe_core->populate_subtree(_tree->subtree(_root_path / "tx_fe_corrections" / 0));

    UHD_LOG_TRACE(unique_id(), "Initializing RX frontend DSP core...")
    _rx_fe_core = rx_frontend_core_3000::make(_get_ctrl(0), regs::sr_addr(RX_FE_BASE));
    _rx_fe_core->set_adc_rate(_master_clock_rate);
    _rx_fe_core->set_dc_offset(rx_frontend_core_3000::DEFAULT_DC_OFFSET_VALUE);
    _rx_fe_core->set_dc_offset_auto(rx_frontend_core_3000::DEFAULT_DC_OFFSET_ENABLE);
    _rx_fe_core->set_iq_balance(rx_frontend_core_3000::DEFAULT_IQ_BALANCE_VALUE);
    _rx_fe_core->populate_subtree(_tree->subtree(_root_path / "rx_fe_corrections" / 0));

    UHD_LOG_TRACE(unique_id(), "Writing initial gain values...");
    set_tx_gain(RHODIUM_DEFAULT_GAIN, 0);
    set_tx_lo_gain(RHODIUM_DEFAULT_LO_GAIN, RHODIUM_LO1, 0);
    set_rx_gain(RHODIUM_DEFAULT_GAIN, 0);
    set_rx_lo_gain(RHODIUM_DEFAULT_LO_GAIN, RHODIUM_LO1, 0);

    UHD_LOG_TRACE(unique_id(), "Initializing TX LO...");
    _tx_lo = lmx2592_iface::make(
        _generate_write_spi(this->_spi, SEN_TX_LO, _get_tx_lo_spi_config()),
        _generate_read_spi(this->_spi, SEN_TX_LO, _get_tx_lo_spi_config()));

    UHD_LOG_TRACE(unique_id(), "Writing initial TX LO state...");
    _tx_lo->set_reference_frequency(RHODIUM_LO1_REF_FREQ);
    _tx_lo->set_mash_order(RHODIUM_DEFAULT_MASH_ORDER);

    UHD_LOG_TRACE(unique_id(), "Initializing RX LO...");
    _rx_lo = lmx2592_iface::make(
        _generate_write_spi(this->_spi, SEN_RX_LO, _get_rx_lo_spi_config()),
        _generate_read_spi(this->_spi, SEN_RX_LO, _get_rx_lo_spi_config()));

    UHD_LOG_TRACE(unique_id(), "Writing initial RX LO state...");
    _rx_lo->set_reference_frequency(RHODIUM_LO1_REF_FREQ);
    _rx_lo->set_mash_order(RHODIUM_DEFAULT_MASH_ORDER);

    UHD_LOG_TRACE(unique_id(), "Initializing GPIOs...");
    _gpio =
        usrp::gpio_atr::gpio_atr_3000::make(
            _get_ctrl(0),
            regs::sr_addr(regs::GPIO),
            regs::rb_addr(regs::RB_DB_GPIO)
        );
    _gpio->set_atr_mode(
        usrp::gpio_atr::MODE_ATR, // Enable ATR mode for Rhodium bits
        RHODIUM_GPIO_MASK
    );
    _gpio->set_atr_mode(
        usrp::gpio_atr::MODE_GPIO, // Disable ATR mode for unused bits
        ~RHODIUM_GPIO_MASK
    );
    _gpio->set_gpio_ddr(
        usrp::gpio_atr::DDR_OUTPUT, // Make all GPIOs outputs
        usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL
    );

    UHD_LOG_TRACE(unique_id(), "Initializing front-panel GPIO control...")
    _fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(
        _get_ctrl(0), regs::sr_addr(regs::FP_GPIO), regs::rb_addr(regs::RB_FP_GPIO));

    UHD_LOG_TRACE(unique_id(), "Set initial ATR values...");
    _update_atr(RHODIUM_DEFAULT_TX_ANTENNA, TX_DIRECTION);
    _update_atr(RHODIUM_DEFAULT_RX_ANTENNA, RX_DIRECTION);

    // Updating the TX frequency path may include an update to SW10, which is
    // GPIO controlled, so this must follow CPLD and GPIO initialization
    UHD_LOG_TRACE(unique_id(), "Writing initial switch values...");
    _update_tx_freq_switches(RHODIUM_DEFAULT_FREQ);
    _update_rx_freq_switches(RHODIUM_DEFAULT_FREQ);

    // Antenna setting requires both CPLD and GPIO control
    UHD_LOG_TRACE(unique_id(), "Setting initial antenna settings");
    _update_tx_output_switches(RHODIUM_DEFAULT_TX_ANTENNA);
    _update_rx_input_switches(RHODIUM_DEFAULT_RX_ANTENNA);

    UHD_LOG_TRACE(unique_id(), "Checking for existence of LO Distribution board");
    _lo_dist_present = _rpcc->request_with_token<bool>(_rpc_prefix + "is_lo_dist_present");
    UHD_LOG_DEBUG(unique_id(), str(boost::format("LO distribution board is%s present") % (_lo_dist_present ? "" : " NOT")));
}

void rhodium_radio_ctrl_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree,
    const size_t chan_idx
) {
    const fs_path tx_fe_path = fs_path("tx_frontends") / chan_idx;
    const fs_path rx_fe_path = fs_path("rx_frontends") / chan_idx;
    UHD_LOG_TRACE(unique_id(),
        "Adding non-RFNoC block properties for channel " << chan_idx <<
        " to prop tree path " << tx_fe_path << " and " << rx_fe_path);
    // TX Standard attributes
    subtree->create<std::string>(tx_fe_path / "name")
        .set(str(boost::format("Rhodium")))
    ;
    subtree->create<std::string>(tx_fe_path / "connection")
        .add_coerced_subscriber([this](const std::string& conn){
            this->_set_tx_fe_connection(conn);
        })
        .set_publisher([this](){
            return this->_get_tx_fe_connection();
        })
    ;
    subtree->create<device_addr_t>(tx_fe_path / "tune_args")
        .set(device_addr_t())
    ;
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name")
        .set(str(boost::format("Rhodium")))
    ;
    subtree->create<std::string>(rx_fe_path / "connection")
        .add_coerced_subscriber([this](const std::string& conn){
            this->_set_rx_fe_connection(conn);
        })
        .set_publisher([this](){
            return this->_get_rx_fe_connection();
        })
    ;
    subtree->create<device_addr_t>(rx_fe_path / "tune_args")
        .set(device_addr_t())
    ;
    // TX Antenna
    subtree->create<std::string>(tx_fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string &ant){
            this->set_tx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_tx_antenna(chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(tx_fe_path / "antenna" / "options")
        .set(RHODIUM_TX_ANTENNAS)
        .add_coerced_subscriber([](const std::vector<std::string> &){
            throw uhd::runtime_error(
                    "Attempting to update antenna options!");
        })
    ;
    // RX Antenna
    subtree->create<std::string>(rx_fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string &ant){
            this->set_rx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_rx_antenna(chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(rx_fe_path / "antenna" / "options")
        .set(RHODIUM_RX_ANTENNAS)
        .add_coerced_subscriber([](const std::vector<std::string> &){
            throw uhd::runtime_error(
                "Attempting to update antenna options!");
        })
    ;
    // TX frequency
    subtree->create<double>(tx_fe_path / "freq" / "value")
        .set_coercer([this, chan_idx](const double freq){
            return this->set_tx_frequency(freq, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_tx_frequency(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "freq" / "range")
        .set(meta_range_t(RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ, 1.0))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update freq range!");
        })
    ;
    // RX frequency
    subtree->create<double>(rx_fe_path / "freq" / "value")
        .set_coercer([this, chan_idx](const double freq){
            return this->set_rx_frequency(freq, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_rx_frequency(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "freq" / "range")
        .set(meta_range_t(RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ, 1.0))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update freq range!");
        })
    ;
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set_coercer([this, chan_idx](const double bw){
            return this->set_tx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_tx_bandwidth(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(RHODIUM_DEFAULT_BANDWIDTH, RHODIUM_DEFAULT_BANDWIDTH))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update bandwidth range!");
        })
    ;
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set_coercer([this, chan_idx](const double bw){
            return this->set_rx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_rx_bandwidth(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(RHODIUM_DEFAULT_BANDWIDTH, RHODIUM_DEFAULT_BANDWIDTH))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update bandwidth range!");
        })
    ;
    // TX gains
    subtree->create<double>(tx_fe_path / "gains" / "all" / "value")
        .set_coercer([this, chan_idx](const double gain){
            return this->set_tx_gain(gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return radio_ctrl_impl::get_tx_gain(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_gain_range(TX_DIRECTION);
        })
    ;

    subtree->create<std::vector<std::string>>(tx_fe_path / "gains/all/profile/options")
            .set(RHODIUM_GP_OPTIONS);

    subtree->create<std::string>(tx_fe_path / "gains/all/profile/value")
        .set_coercer([this](const std::string& profile){
            std::string return_profile = profile;
            if (!uhd::has(RHODIUM_GP_OPTIONS, profile))
            {
                return_profile = "default";
            }
            _gain_profile[TX_DIRECTION] = return_profile;
            return return_profile;
        })
        .set_publisher([this](){
            return _gain_profile[TX_DIRECTION];
        })
    ;

    // RX gains
    subtree->create<double>(rx_fe_path / "gains" / "all" / "value")
        .set_coercer([this, chan_idx](const double gain){
            return this->set_rx_gain(gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return radio_ctrl_impl::get_rx_gain(chan_idx);
        })
    ;

    subtree->create<meta_range_t>(rx_fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_gain_range(RX_DIRECTION);
        })
    ;

    subtree->create<std::vector<std::string> >(rx_fe_path / "gains/all/profile/options")
            .set(RHODIUM_GP_OPTIONS);

    subtree->create<std::string>(rx_fe_path / "gains/all/profile/value")
        .set_coercer([this](const std::string& profile){
            std::string return_profile = profile;
            if (!uhd::has(RHODIUM_GP_OPTIONS, profile))
            {
                return_profile = "default";
            }
            _gain_profile[RX_DIRECTION] = return_profile;
            return return_profile;
        })
        .set_publisher([this](){
            return _gain_profile[RX_DIRECTION];
        })
    ;

    // TX LO lock sensor
    subtree->create<sensor_value_t>(tx_fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false,  "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t &){
            throw uhd::runtime_error(
                "Attempting to write to sensor!");
        })
        .set_publisher([this](){
            return sensor_value_t(
                "all_los",
                this->get_lo_lock_status(TX_DIRECTION),
                "locked", "unlocked"
            );
        })
    ;
    // RX LO lock sensor
    subtree->create<sensor_value_t>(rx_fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false,  "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t &){
            throw uhd::runtime_error(
                "Attempting to write to sensor!");
        })
        .set_publisher([this](){
            return sensor_value_t(
                "all_los",
                this->get_lo_lock_status(RX_DIRECTION),
                "locked", "unlocked"
            );
        })
    ;
    //LO Specific
    //RX LO
    //RX LO1 Frequency
    subtree->create<double>(rx_fe_path / "los"/RHODIUM_LO1/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double freq){
            return this->set_rx_lo_freq(freq, RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "los"/RHODIUM_LO1/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(RHODIUM_LO1, chan_idx);
        })
    ;
    //RX LO1 Source
    subtree->create<std::vector<std::string>>(rx_fe_path / "los"/RHODIUM_LO1/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_sources(RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::string>(rx_fe_path / "los"/RHODIUM_LO1/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_rx_lo_source(src, RHODIUM_LO1,chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_source(RHODIUM_LO1, chan_idx);
        })
    ;
    //RX LO1 Export
    subtree->create<bool>(rx_fe_path / "los"/RHODIUM_LO1/"export")
        .add_coerced_subscriber([this,chan_idx](bool enabled){
            this->set_rx_lo_export_enabled(enabled, RHODIUM_LO1, chan_idx);
        })
    ;
    //RX LO1 Gain
    subtree->create<double>(rx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_GAIN / "value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_gain(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double gain){
            return this->set_rx_lo_gain(gain, RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_GAIN / "range")
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_lo_gain_range();
        })
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error("Attempting to update LO gain range!");
        })
    ;
    //RX LO1 Output Power
    subtree->create<double>(rx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_POWER / "value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_power(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double gain){
            return this->set_rx_lo_power(gain, RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_POWER / "range")
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_lo_power_range();
        })
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error("Attempting to update LO output power range!");
        })
    ;
    //RX LO2 Frequency
    subtree->create<double>(rx_fe_path / "los"/RHODIUM_LO2/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq(RHODIUM_LO2, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_rx_lo_freq(freq, RHODIUM_LO2, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "los"/RHODIUM_LO2/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(RHODIUM_LO2, chan_idx);
        })
    ;
    //RX LO2 Source
    subtree->create<std::vector<std::string>>(rx_fe_path / "los"/RHODIUM_LO2/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_sources(RHODIUM_LO2, chan_idx);
        })
    ;
    subtree->create<std::string>(rx_fe_path / "los"/RHODIUM_LO2/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_rx_lo_source(src, RHODIUM_LO2, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_source(RHODIUM_LO2, chan_idx);
        })
    ;
    //RX LO2 Export
    subtree->create<bool>(rx_fe_path / "los"/RHODIUM_LO2/"export")
            .add_coerced_subscriber([this,chan_idx](bool enabled){
              this->set_rx_lo_export_enabled(enabled, RHODIUM_LO2, chan_idx);
        });
    //RX ALL LOs
    subtree->create<std::string>(rx_fe_path / "los" / ALL_LOS / "source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src) {
            this->set_rx_lo_source(src, ALL_LOS, chan_idx);
        })
        .set_publisher([this,chan_idx]() {
            return this->get_rx_lo_source(ALL_LOS, chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(rx_fe_path / "los" / ALL_LOS / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_rx_lo_sources(ALL_LOS, chan_idx);
        })
    ;
    subtree->create<bool>(rx_fe_path / "los" / ALL_LOS / "export")
        .add_coerced_subscriber([this,chan_idx](bool enabled){
            this->set_rx_lo_export_enabled(enabled, ALL_LOS, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_export_enabled(ALL_LOS, chan_idx);
        })
    ;
    //TX LO
    //TX LO1 Frequency
    subtree->create<double>(tx_fe_path / "los"/RHODIUM_LO1/"freq/value ")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_tx_lo_freq(freq, RHODIUM_LO1, chan_idx);
        })
    ;
     subtree->create<meta_range_t>(tx_fe_path / "los"/RHODIUM_LO1/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(RHODIUM_LO1, chan_idx);
        })
    ;
    //TX LO1 Source
    subtree->create<std::vector<std::string>>(tx_fe_path / "los"/RHODIUM_LO1/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_sources(RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::string>(tx_fe_path / "los"/RHODIUM_LO1/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_tx_lo_source(src, RHODIUM_LO1, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_source(RHODIUM_LO1, chan_idx);
        })
    ;
    //TX LO1 Export
    subtree->create<bool>(tx_fe_path / "los"/RHODIUM_LO1/"export")
            .add_coerced_subscriber([this,chan_idx](bool enabled){
              this->set_tx_lo_export_enabled(enabled, RHODIUM_LO1, chan_idx);
            })
    ;
    //TX LO1 Gain
    subtree->create<double>(tx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_GAIN / "value")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_gain(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double gain){
            return this->set_tx_lo_gain(gain, RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_GAIN / "range")
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_lo_gain_range();
        })
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error("Attempting to update LO gain range!");
        })
    ;
    //TX LO1 Output Power
    subtree->create<double>(tx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_POWER / "value")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_power(RHODIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double gain){
            return this->set_tx_lo_power(gain, RHODIUM_LO1, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "los" /RHODIUM_LO1/ "gains" / RHODIUM_LO_POWER / "range")
        .set_publisher([](){
            return rhodium_radio_ctrl_impl::_get_lo_power_range();
        })
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error("Attempting to update LO output power range!");
        })
    ;
    //TX LO2 Frequency
    subtree->create<double>(tx_fe_path / "los"/RHODIUM_LO2/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq(RHODIUM_LO2, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_tx_lo_freq(freq, RHODIUM_LO2, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "los"/RHODIUM_LO2/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq_range(RHODIUM_LO2,chan_idx);
        })
    ;
    //TX LO2 Source
    subtree->create<std::vector<std::string>>(tx_fe_path / "los"/RHODIUM_LO2/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_sources(RHODIUM_LO2, chan_idx);
        })
    ;
    subtree->create<std::string>(tx_fe_path / "los"/RHODIUM_LO2/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_tx_lo_source(src, RHODIUM_LO2, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_source(RHODIUM_LO2, chan_idx);
        })
    ;
    //TX LO2 Export
    subtree->create<bool>(tx_fe_path / "los"/RHODIUM_LO2/"export")
        .add_coerced_subscriber([this,chan_idx](bool enabled){
            this->set_tx_lo_export_enabled(enabled, RHODIUM_LO2, chan_idx);
        })
    ;
    //TX ALL LOs
    subtree->create<std::string>(tx_fe_path / "los" / ALL_LOS / "source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src) {
            this->set_tx_lo_source(src, ALL_LOS, chan_idx);
        })
        .set_publisher([this,chan_idx]() {
            return this->get_tx_lo_source(ALL_LOS, chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(tx_fe_path / "los" / ALL_LOS / "source/options")
        .set_publisher([this, chan_idx]() {
            return this->get_tx_lo_sources(ALL_LOS, chan_idx);
        })
    ;
    subtree->create<bool>(tx_fe_path / "los" / ALL_LOS / "export")
        .add_coerced_subscriber([this,chan_idx](bool enabled){
            this->set_tx_lo_export_enabled(enabled, ALL_LOS, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_export_enabled(ALL_LOS, chan_idx);
        })
    ;

    //LO Distribution Output Ports
    if (_lo_dist_present) {
        for (const auto& port : LO_OUTPUT_PORT_NAMES) {
            subtree->create<bool>(tx_fe_path / "los" / RHODIUM_LO1 / "lo_distribution" / port / "export")
                .add_coerced_subscriber([this, chan_idx, port](bool enabled) {
                    this->set_tx_lo_output_enabled(enabled, port, chan_idx);
                })
                .set_publisher([this, chan_idx, port]() {
                    return this->get_tx_lo_output_enabled(port, chan_idx);
                })
            ;
            subtree->create<bool>(rx_fe_path / "los" / RHODIUM_LO1 / "lo_distribution" / port / "export")
                .add_coerced_subscriber([this, chan_idx, port](bool enabled) {
                    this->set_rx_lo_output_enabled(enabled, port, chan_idx);
                })
                .set_publisher([this, chan_idx, port]() {
                    return this->get_rx_lo_output_enabled(port, chan_idx);
                })
            ;
        }
    }
}

void rhodium_radio_ctrl_impl::_init_prop_tree()
{
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    this->_init_frontend_subtree(_tree->subtree(fe_base), 0);

    // legacy EEPROM paths
    auto eeprom_get = [this]() {
        auto eeprom     = dboard_eeprom_t();
        eeprom.id       = boost::lexical_cast<uint16_t>(_dboard_info.at("pid"));
        eeprom.revision = _dboard_info.at("rev");
        eeprom.serial   = _dboard_info.at("serial");
        return eeprom;
    };

    auto eeprom_set = [](dboard_eeprom_t) {
        throw uhd::not_implemented_error("Setting DB EEPROM from this interface not implemented");
    };

    _tree->create<dboard_eeprom_t>(fe_base / "rx_eeprom")
        .set_publisher(eeprom_get)
        .add_coerced_subscriber(eeprom_set);

    _tree->create<dboard_eeprom_t>(fe_base / "tx_eeprom")
        .set_publisher(eeprom_get)
        .add_coerced_subscriber(eeprom_set);

    // EEPROM paths subject to change FIXME
    _tree->create<eeprom_map_t>(_root_path / "eeprom")
        .set(eeprom_map_t());

    _tree->create<int>("rx_codecs" / _radio_slot / "gains");
    _tree->create<int>("tx_codecs" / _radio_slot / "gains");
    _tree->create<std::string>("rx_codecs" / _radio_slot / "name").set("ad9695-625");
    _tree->create<std::string>("tx_codecs" / _radio_slot / "name").set("dac37j82");

    // The tick_rate is equivalent to the master clock rate of the DB in slot A
    if (_radio_slot == "A")
    {
        UHD_ASSERT_THROW(!_tree->exists("tick_rate"));
        // set_rate sets the clock rate of the entire device, not just this DB,
        // so only add DB A's set and get functions to the tree.
        _tree->create<double>("tick_rate")
            .set_publisher([this](){ return this->get_rate(); })
            .add_coerced_subscriber([this](double rate) { return this->set_rate(rate); })
        ;
    }

    // *****FP_GPIO************************
    for (const auto& attr : usrp::gpio_atr::gpio_attr_map) {
        if (not _tree->exists(fs_path("gpio") / "FP0" / attr.second)) {
            switch (attr.first) {
                case usrp::gpio_atr::GPIO_SRC:
                    // This is not really the place to configure the source
                    // setting of the GPIO, don't have a better place to put this.
                    // Note: In UHD 4.0, this will move to the mb_controller
                    // object.
                    _tree
                        ->create<std::vector<std::string>>(
                            fs_path("gpio") / "FP0" / attr.second)
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
                                            extract_port_number(str_val[i], _tree);
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
                    _tree
                        ->create<std::vector<std::string>>(
                            fs_path("gpio") / "FP0" / attr.second)
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
                    _tree->create<uint32_t>(fs_path("gpio") / "FP0" / attr.second)
                        .set_publisher([this]() { return _fp_gpio->read_gpio(); });
                } break;
                default:
                    _tree->create<uint32_t>(fs_path("gpio") / "FP0" / attr.second)
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
                    _tree
                        ->access<std::vector<std::string>>(
                            fs_path("gpio") / "FP0" / attr.second)
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
                    _tree->access<uint32_t>(fs_path("gpio") / "FP0" / attr.second)
                        .set(0)
                        .add_coerced_subscriber([this, attr](const uint32_t val) {
                            _fp_gpio->set_gpio_attr(attr.first, val);
                        });
            }
        }
    }
}

void rhodium_radio_ctrl_impl::_init_mpm_sensors(
        const direction_t dir,
        const size_t chan_idx
) {
    const std::string trx = (dir == RX_DIRECTION) ? "RX" : "TX";
    const fs_path fe_path =
        fs_path("dboards") / _radio_slot /
        (dir == RX_DIRECTION ? "rx_frontends" : "tx_frontends") / chan_idx;
    auto sensor_list =
        _rpcc->request_with_token<std::vector<std::string>>(
                this->_rpc_prefix + "get_sensors", trx);
    UHD_LOG_TRACE(unique_id(),
        "Chan " << chan_idx << ": Found "
        << sensor_list.size() << " " << trx << " sensors.");
    for (const auto &sensor_name : sensor_list) {
        UHD_LOG_TRACE(unique_id(),
            "Adding " << trx << " sensor " << sensor_name);
        _tree->create<sensor_value_t>(fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t &){
                throw uhd::runtime_error(
                    "Attempting to write to sensor!");
            })
            .set_publisher([this, trx, sensor_name, chan_idx](){
                return sensor_value_t(
                    this->_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
                        this->_rpc_prefix + "get_sensor",
                            trx, sensor_name, chan_idx)
                );
            })
        ;
    }
}

