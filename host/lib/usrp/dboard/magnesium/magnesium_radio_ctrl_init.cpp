//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_constants.hpp"
#include "spi_core_3000.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/transport/chdr.hpp>
#include <vector>
#include <string>

using namespace uhd;
using namespace uhd::rfnoc;

namespace {
    enum slave_select_t {
        SEN_CPLD = 1,
        SEN_TX_LO = 2,
        SEN_RX_LO = 4,
        SEN_PHASE_DAC = 8
    };

    constexpr double MAGNESIUM_DEFAULT_FREQ         = 2.5e9; // Hz
    constexpr double MAGNESIUM_DEFAULT_BANDWIDTH    = 100e6; // Hz
    constexpr char   MAGNESIUM_DEFAULT_RX_ANTENNA[] = "RX2";
    constexpr char   MAGNESIUM_DEFAULT_TX_ANTENNA[] = "TX/RX";

    //! Magnesium gain profile options
    const std::vector<std::string> MAGNESIUM_GP_OPTIONS = {
        "manual",
        "default"
    };
}

void magnesium_radio_ctrl_impl::_init_defaults()
{
    UHD_LOG_TRACE(unique_id(), "Initializing defaults...");
    const size_t num_rx_chans = get_output_ports().size();
    const size_t num_tx_chans = get_input_ports().size();

    UHD_LOG_TRACE(unique_id(),
            "Num TX chans: " << num_tx_chans
            << " Num RX chans: " << num_rx_chans);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(MAGNESIUM_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(0, chan);
        radio_ctrl_impl::set_rx_antenna(MAGNESIUM_DEFAULT_RX_ANTENNA, chan);
        radio_ctrl_impl::set_rx_bandwidth(MAGNESIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_ctrl_impl::set_tx_frequency(MAGNESIUM_DEFAULT_FREQ, chan);
        radio_ctrl_impl::set_tx_gain(0, chan);
        radio_ctrl_impl::set_tx_antenna(MAGNESIUM_DEFAULT_TX_ANTENNA, chan);
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
}

void magnesium_radio_ctrl_impl::_init_peripherals()
{
    UHD_LOG_TRACE(unique_id(), "Initializing peripherals...");
    fs_path cpld_path  = _root_path.branch_path()
        / str(boost::format("Radio_%d") % ((get_block_id().get_block_count()/2)*2))
        / "cpld";
    fs_path rx_lo_path  = _root_path.branch_path()
        / str(boost::format("Radio_%d") % ((get_block_id().get_block_count()/2)*2))
        / "rx_lo";
    fs_path tx_lo_path  = _root_path.branch_path()
        / str(boost::format("Radio_%d") % ((get_block_id().get_block_count()/2)*2))
        / "tx_lo";
    // TODO: When we move back to 2 chans per RFNoC block, this needs to be
    // non-conditional, and the else-branch goes away:
    if (_master) {
        UHD_LOG_TRACE(unique_id(), "Initializing SPI core...");
        _spi = spi_core_3000::make(_get_ctrl(0),
            radio_ctrl_impl::regs::sr_addr(radio_ctrl_impl::regs::SPI),
            radio_ctrl_impl::regs::RB_SPI);
    } else {
        UHD_LOG_TRACE(unique_id(), "Not a master radio, no SPI core.");
    }

    UHD_LOG_TRACE(unique_id(), "Initializing CPLD...");
    UHD_LOG_TRACE(unique_id(), "CPLD path: " << cpld_path);
    if (not _tree->exists(cpld_path)) {
        UHD_LOG_TRACE(unique_id(), "Creating new CPLD object...");
        spi_config_t spi_config;
        spi_config.use_custom_divider = true;
        spi_config.divider = 125;
        spi_config.mosi_edge = spi_config_t::EDGE_RISE;
        spi_config.miso_edge = spi_config_t::EDGE_FALL;
        UHD_LOG_TRACE(unique_id(), "Making CPLD object...");
        _cpld = std::make_shared<magnesium_cpld_ctrl>(
            [this, spi_config](const uint32_t transaction){ // Write functor
                this->_spi->write_spi(
                    SEN_CPLD,
                    spi_config,
                    transaction,
                    24
                );
            },
            [this, spi_config](const uint32_t transaction){ // Read functor
                return this->_spi->read_spi(
                    SEN_CPLD,
                    spi_config,
                    transaction,
                    24
                );
            }
        );
        _update_atr_switches(
            magnesium_cpld_ctrl::BOTH,
            DX_DIRECTION,
            radio_ctrl_impl::get_rx_antenna(0)
        );
        _tree->create<magnesium_cpld_ctrl::sptr>(cpld_path).set(_cpld);
    } else {
        UHD_LOG_TRACE(unique_id(), "Reusing someone else's CPLD object...");
        _cpld = _tree->access<magnesium_cpld_ctrl::sptr>(cpld_path).get();
    }

    // TODO: Same comment as above applies
    if (_master) {
        UHD_LOG_TRACE(unique_id(), "Initializing TX LO...");
        _tx_lo = adf435x_iface::make_adf4351(
            [this](const std::vector<uint32_t> transactions){
                for (const uint32_t transaction: transactions) {
                    this->_spi->write_spi(
                        SEN_TX_LO,
                        spi_config_t::EDGE_RISE,
                        transaction,
                        32
                    );
                }
            }
        );
        UHD_LOG_TRACE(unique_id(), "Initializing RX LO...");
        _rx_lo = adf435x_iface::make_adf4351(
            [this](const std::vector<uint32_t> transactions){
                for (const uint32_t transaction: transactions) {
                    this->_spi->write_spi(
                        SEN_RX_LO,
                        spi_config_t::EDGE_RISE,
                        transaction,
                        32
                    );
                }
            }
        );
    } else {
        UHD_LOG_TRACE(unique_id(), "Not a master radio, no LOs.");
    }

    _gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < _get_num_radios(); radio_idx++) {
        UHD_LOG_TRACE(unique_id(),
            "Initializing GPIOs for channel " << radio_idx);
        _gpio.emplace_back(
            usrp::gpio_atr::gpio_atr_3000::make(
                _get_ctrl(radio_idx),
                regs::sr_addr(regs::GPIO),
                regs::RB_DB_GPIO
            )
        );
        // DSA and AD9371 gain bits do *not* toggle on ATR modes. If we ever
        // connect anything else to this core, we might need to set_atr_mode()
        // to MODE_ATR on those bits. For now, all bits simply do what they're
        // told, and don't toggle on RX/TX state changes.
         _gpio.back()->set_atr_mode(
             usrp::gpio_atr::MODE_GPIO, // Disable ATR mode
             usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL
         );
         _gpio.back()->set_gpio_ddr(
            usrp::gpio_atr::DDR_OUTPUT, // Make all GPIOs outputs
            usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL
        );
    }
    if (get_block_id().get_block_count() == FPGPIO_MASTER_RADIO) {
        UHD_LOG_TRACE(unique_id(), "Initializing front-panel GPIO control...")
        _fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(
                _get_ctrl(0), regs::sr_addr(regs::FP_GPIO), regs::RB_FP_GPIO);
    }
}

void magnesium_radio_ctrl_impl::_init_frontend_subtree(
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
        .set(str(boost::format("Magnesium")))
    ;
    subtree->create<std::string>(tx_fe_path / "connection")
        .set("IQ")
    ;
    // RX Standard attributes
    subtree->create<std::string>(rx_fe_path / "name")
        .set(str(boost::format("Magnesium")))
    ;
    subtree->create<std::string>(rx_fe_path / "connection")
        .set("IQ")
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
        .set({MAGNESIUM_DEFAULT_TX_ANTENNA})
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
        .set(MAGNESIUM_RX_ANTENNAS)
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
        .set(meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ, 1.0))
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
        .set(meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ, 1.0))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update freq range!");
        })
    ;
    // TX bandwidth
    subtree->create<double>(tx_fe_path / "bandwidth" / "value")
        .set(AD9371_TX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw){
            return this->set_tx_bandwidth(bw, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(AD9371_TX_MIN_BANDWIDTH, AD9371_TX_MAX_BANDWIDTH))
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update bandwidth range!");
        })
    ;
    // RX bandwidth
    subtree->create<double>(rx_fe_path / "bandwidth" / "value")
        .set(AD9371_RX_MAX_BANDWIDTH)
        .set_coercer([this, chan_idx](const double bw){
            return this->set_rx_bandwidth(bw, chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(AD9371_RX_MIN_BANDWIDTH, AD9371_RX_MAX_BANDWIDTH))
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
        .set_publisher([this, chan_idx](){
            if (_gain_profile[TX_DIRECTION] == "manual") {
                return meta_range_t(0.0, 0.0, 0.0);
            }else{
                return meta_range_t(ALL_TX_MIN_GAIN, ALL_TX_MAX_GAIN, ALL_TX_GAIN_STEP);
            }
        })
    ;

    subtree->create<std::vector<std::string> >(tx_fe_path / "gains/all/profile/options")
            .set(boost::assign::list_of("manual")("default"));

    subtree->create<std::string>(tx_fe_path / "gains/all/profile/value")
        .set_coercer([this, chan_idx](const std::string& profile){
            std::string return_profile = profile;
            if (std::find(MAGNESIUM_GP_OPTIONS.begin(),
                         MAGNESIUM_GP_OPTIONS.end(),
                         profile
                         ) == MAGNESIUM_GP_OPTIONS.end())
            {
                return_profile = "default";
            }
            _gain_profile[TX_DIRECTION] = return_profile;
            return return_profile;
        })
        .set_publisher([this, chan_idx](){
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
        .set_publisher([this, chan_idx](){
            if (_gain_profile[RX_DIRECTION] == "manual") {
                return meta_range_t(0.0, 0.0, 0.0);
            }else{
                return meta_range_t(ALL_RX_MIN_GAIN, ALL_RX_MAX_GAIN, ALL_RX_GAIN_STEP);
            }
        })
    ;

    subtree->create<std::vector<std::string> >(rx_fe_path / "gains/all/profile/options")
            .set(MAGNESIUM_GP_OPTIONS);

    subtree->create<std::string>(rx_fe_path / "gains/all/profile/value")
        .set_coercer([this, chan_idx](const std::string& profile){
            std::string return_profile = profile;
            if (std::find(MAGNESIUM_GP_OPTIONS.begin(),
                         MAGNESIUM_GP_OPTIONS.end(),
                         profile
                         ) == MAGNESIUM_GP_OPTIONS.end())
            {
                return_profile = "default";
            }
            _gain_profile[RX_DIRECTION] = return_profile;
            return return_profile;
        })
        .set_publisher([this, chan_idx](){
            return _gain_profile[RX_DIRECTION];
        })
    ;

    // TX mykonos attenuation
    subtree->create<double>(tx_fe_path / "gains" / MAGNESIUM_GAIN1 / "value")
        .set_coercer([this, chan_idx](const double gain){
            return _set_tx_gain(MAGNESIUM_GAIN1, gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->_get_tx_gain(MAGNESIUM_GAIN1, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(tx_fe_path / "gains" / MAGNESIUM_GAIN1 / "range")
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[TX_DIRECTION] == "manual") {
                return meta_range_t(AD9371_MIN_TX_GAIN, AD9371_MAX_TX_GAIN, AD9371_TX_GAIN_STEP);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;
     // TX DSA
    subtree->create<double>(tx_fe_path / "gains" / MAGNESIUM_GAIN2 / "value")
        .set_coercer([this, chan_idx](const double gain){
            return this->_set_tx_gain(MAGNESIUM_GAIN2, gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->_get_tx_gain(MAGNESIUM_GAIN2, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(tx_fe_path / "gains" / MAGNESIUM_GAIN2 / "range")
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[TX_DIRECTION] == "manual") {
                return meta_range_t(DSA_MIN_GAIN, DSA_MAX_GAIN, DSA_GAIN_STEP);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;
     //TX amp
    subtree->create<double>(tx_fe_path / "gains" / MAGNESIUM_AMP / "value")
        .set_coercer([this, chan_idx](const double gain) {
            return this->_set_tx_gain(MAGNESIUM_AMP, gain, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->_get_tx_gain(MAGNESIUM_AMP, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(tx_fe_path / "gains" / MAGNESIUM_AMP / "range")
        .add_coerced_subscriber([](const meta_range_t &) {
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[TX_DIRECTION] == "manual") {
                return meta_range_t(AMP_MIN_GAIN, AMP_MAX_GAIN, AMP_GAIN_STEP);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;

    // RX mykonos attenuation
    subtree->create<double>(rx_fe_path / "gains" / MAGNESIUM_GAIN1 / "value")
        .set_coercer([this, chan_idx](const double gain){
                UHD_VAR(gain);
            return this->_set_rx_gain(MAGNESIUM_GAIN1, gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->_get_rx_gain(MAGNESIUM_GAIN1, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(rx_fe_path / "gains" / MAGNESIUM_GAIN1 / "range")
        .add_coerced_subscriber([](const meta_range_t &) {
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[RX_DIRECTION] == "manual") {
                return meta_range_t(AD9371_MIN_RX_GAIN, AD9371_MAX_RX_GAIN, AD9371_RX_GAIN_STEP);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;
    //RX DSA
    subtree->create<double>(rx_fe_path / "gains" / MAGNESIUM_GAIN2 / "value")
        .set_coercer([this, chan_idx](const double gain) {
            UHD_VAR(gain);
            return this->_set_rx_gain(MAGNESIUM_GAIN2, gain, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->_get_rx_gain(MAGNESIUM_GAIN2, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(rx_fe_path / "gains" / MAGNESIUM_GAIN2 / "range")
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[RX_DIRECTION] == "manual") {
                return meta_range_t(DSA_MIN_GAIN, DSA_MAX_GAIN, DSA_MAX_GAIN);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;

    //RX amp
    subtree->create<double>(rx_fe_path / "gains" / MAGNESIUM_AMP / "value")
        .set_coercer([this, chan_idx](const double gain) {
            return this->_set_rx_gain(MAGNESIUM_AMP, gain, chan_idx);
        })
        .set_publisher([this, chan_idx]() {
            return this->_get_rx_gain(MAGNESIUM_AMP, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(rx_fe_path / "gains" / MAGNESIUM_AMP / "range")
        .add_coerced_subscriber([](const meta_range_t &) {
            throw uhd::runtime_error(
                "Attempting to update gain range!");
        })
        .set_publisher([this, chan_idx](){
            if (_gain_profile[RX_DIRECTION] == "manual") {
                return meta_range_t(AMP_MIN_GAIN, AMP_MAX_GAIN, AMP_GAIN_STEP);
            }else{
                return meta_range_t(0.0, 0.0, 0.0);
            }
        })
    ;

    // TX LO lock sensor //////////////////////////////////////////////////////
    // Note: The lowband and AD9371 LO lock sensors are generated
    // programmatically in set_rpc_client(). The actual lo_locked publisher is
    // also set there.
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
    // RX LO lock sensor (see not on TX LO lock sensor)
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
    subtree->create<meta_range_t>(rx_fe_path / "los"/MAGNESIUM_LO1/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(rx_fe_path / "los"/MAGNESIUM_LO1/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_sources(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::string>(rx_fe_path / "los"/MAGNESIUM_LO1/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_rx_lo_source(src, MAGNESIUM_LO1,chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_source(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<double>(rx_fe_path / "los"/MAGNESIUM_LO1/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq(MAGNESIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](const double freq){
            return this->set_rx_lo_freq(freq, MAGNESIUM_LO1, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(rx_fe_path / "los"/MAGNESIUM_LO2/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(MAGNESIUM_LO2, chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(rx_fe_path / "los"/MAGNESIUM_LO2/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_sources(MAGNESIUM_LO2, chan_idx);
        })
    ;

    subtree->create<std::string>(rx_fe_path / "los"/MAGNESIUM_LO2/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_rx_lo_source(src, MAGNESIUM_LO2, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_source(MAGNESIUM_LO2, chan_idx);
        })
    ;
    subtree->create<double>(rx_fe_path / "los"/MAGNESIUM_LO2/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq(MAGNESIUM_LO2, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_rx_lo_freq(freq, MAGNESIUM_LO2, chan_idx);
        });
    //TX LO
     subtree->create<meta_range_t>(tx_fe_path / "los"/MAGNESIUM_LO1/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_rx_lo_freq_range(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(tx_fe_path / "los"/MAGNESIUM_LO1/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_sources(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<std::string>(tx_fe_path / "los"/MAGNESIUM_LO1/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_tx_lo_source(src, MAGNESIUM_LO1, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_source(MAGNESIUM_LO1, chan_idx);
        })
    ;
    subtree->create<double>(tx_fe_path / "los"/MAGNESIUM_LO1/"freq/value ")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq(MAGNESIUM_LO1, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_tx_lo_freq(freq, MAGNESIUM_LO1, chan_idx);
        })
    ;

    subtree->create<meta_range_t>(tx_fe_path / "los"/MAGNESIUM_LO2/"freq/range")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq_range(MAGNESIUM_LO2,chan_idx);
        })
    ;
    subtree->create<std::vector<std::string>>(tx_fe_path / "los"/MAGNESIUM_LO2/"source/options")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_sources(MAGNESIUM_LO2, chan_idx);
        })
    ;

    subtree->create<std::string>(tx_fe_path / "los"/MAGNESIUM_LO2/"source/value")
        .add_coerced_subscriber([this,chan_idx](std::string src){
            this->set_tx_lo_source(src, MAGNESIUM_LO2, chan_idx);
        })
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_source(MAGNESIUM_LO2, chan_idx);
        })
    ;
    subtree->create<double>(tx_fe_path / "los"/MAGNESIUM_LO2/"freq/value")
        .set_publisher([this,chan_idx](){
            return this->get_tx_lo_freq(MAGNESIUM_LO2, chan_idx);
        })
        .set_coercer([this,chan_idx](double freq){
            return this->set_tx_lo_freq(freq, MAGNESIUM_LO2, chan_idx);
        });
}

void magnesium_radio_ctrl_impl::_init_prop_tree()
{
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    this->_init_frontend_subtree(_tree->subtree(fe_base), 0);
    // TODO: When we go to one radio per dboard, the above if statement goes
    // away, and instead we have something like this:
    /*
     *for (chan_idx = 0; chan_idx < MAGNESIUM_NUM_CHANS; chan_idx++) {
     *    this->_init_frontend_subtree(
     *        _tree->get_subtree(fe_base), chan_idx);
     *}
     */



    // EEPROM paths subject to change FIXME
    _tree->create<eeprom_map_t>(_root_path / "eeprom")
        .set(eeprom_map_t());

    // TODO change codec names
    _tree->create<int>("rx_codecs" / _radio_slot / "gains");
    _tree->create<int>("tx_codecs" / _radio_slot / "gains");
    _tree->create<std::string>("rx_codecs" / _radio_slot / "name").set("AD9371 Dual ADC");
    _tree->create<std::string>("tx_codecs" / _radio_slot / "name").set("AD9371 Dual DAC");

    // TODO remove this dirty hack
    if (not _tree->exists("tick_rate"))
    {
        _tree->create<double>("tick_rate")
            .set_publisher([this](){ return this->get_rate(); })
        ;
    }
}

void magnesium_radio_ctrl_impl::_init_mpm_sensors(
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

