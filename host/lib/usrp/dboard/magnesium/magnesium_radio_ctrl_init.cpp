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
    if (not _tree->exists(rx_lo_path)) {
        _tree->create<adf435x_iface::sptr>(rx_lo_path).set(_rx_lo);
    } else {
        UHD_LOG_TRACE(unique_id(), "Not a master radio. Getting LO from master" );
        _rx_lo = _tree->access<adf435x_iface::sptr>(rx_lo_path).get();
    }
    if (not _tree->exists(tx_lo_path)) {
        _tree->create<adf435x_iface::sptr>(tx_lo_path).set(_tx_lo);
    } else {
        UHD_LOG_TRACE(unique_id(), "Not a master radio. Getting LO from master" );
        _tx_lo = _tree->access<adf435x_iface::sptr>(tx_lo_path).get();
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

void magnesium_radio_ctrl_impl::_init_defaults()
{
    UHD_LOG_TRACE(unique_id(), "Initializing defaults...");
    const size_t num_rx_chans = get_output_ports().size();
    const size_t num_tx_chans = get_input_ports().size();

    UHD_LOG_TRACE(unique_id(),
            "Num TX chans: " << num_tx_chans
            << " Num RX chans: " << num_rx_chans);
    UHD_LOG_TRACE(unique_id(),
            "Setting tick rate to " << MAGNESIUM_TICK_RATE / 1e6 << " MHz");
    radio_ctrl_impl::set_rate(MAGNESIUM_TICK_RATE);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(MAGNESIUM_CENTER_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(0, chan);
        radio_ctrl_impl::set_rx_antenna(MAGNESIUM_DEFAULT_RX_ANTENNA, chan);
        radio_ctrl_impl::set_rx_bandwidth(MAGNESIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_ctrl_impl::set_tx_frequency(MAGNESIUM_CENTER_FREQ, chan);
        radio_ctrl_impl::set_tx_gain(0, chan);
        radio_ctrl_impl::set_tx_antenna(MAGNESIUM_DEFAULT_TX_ANTENNA, chan);
    }
}

void magnesium_radio_ctrl_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree,
    const size_t fe_chan_idx,
    const size_t chan_idx
) {
    const fs_path tx_fe_path = fs_path("tx_frontends") / fe_chan_idx;
    const fs_path rx_fe_path = fs_path("rx_frontends") / fe_chan_idx;
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
        .set_coercer([this, chan_idx](const double bw){
            return this->set_tx_bandwidth(bw, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            //return this->get_tx_bandwidth(chan_idx);
            return 0.0; // FIXME
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "bandwidth" / "range")
        .set(meta_range_t(0.0, 0.0, 0.0)) // FIXME
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
        .set(meta_range_t(0.0, 0.0, 0.0)) // FIXME
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
            return this->get_tx_gain(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(tx_fe_path / "gains" / "all" / "range")
        .set(meta_range_t(0.0, 60.0, 1.0)) // FIXME
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update bandwidth range!");
        })
    ;
    // RX gains
    subtree->create<double>(rx_fe_path / "gains" / "all" / "value")
        .set_coercer([this, chan_idx](const double gain){
                UHD_VAR(gain);
            return this->set_rx_gain(gain, chan_idx);
        })
        .set_publisher([this, chan_idx](){
            return this->get_rx_gain(chan_idx);
        })
    ;
    subtree->create<meta_range_t>(rx_fe_path / "gains" / "all" / "range")
        .set(meta_range_t(0.0, 60.0, 1.0)) // FIXME
        .add_coerced_subscriber([](const meta_range_t &){
            throw uhd::runtime_error(
                "Attempting to update bandwidth range!");
        })
    ;
    // FIXME separate DSA and Myk gains
}

void magnesium_radio_ctrl_impl::_init_prop_tree()
{
    fs_path gain_mode_path =
        _root_path.branch_path()
        / str(boost::format("Radio_%d") % ((get_block_id().get_block_count()/2)*2))
        / "args/0/gain_mode/value";
    UHD_LOG_DEBUG("GAIN_MODE_STRING","Gain mode path " << gain_mode_path);
    std::string gain_mode = _tree->access<std::string>(gain_mode_path).get();
    UHD_LOG_DEBUG("GAIN_MODE_STRING","Gain mode string" << gain_mode);

    /**** Set up legacy compatible properties ******************************/
    // For use with multi_usrp APIs etc.
    // For legacy prop tree init:
    // TODO: determine DB number
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    const std::vector<uhd::direction_t> dir({ RX_DIRECTION, TX_DIRECTION });
    const std::vector<std::string> fe({ "rx_frontends", "tx_frontends" });
    const std::vector<std::string> ant({ "RX" , "TX" });
    //const size_t RX_IDX = 0;
    // const size_t TX_IDX = 1;
    //this->_dsa_set_gain(0.5,0,RX_DIRECTION);
    for (size_t fe_idx = 0; fe_idx < fe.size(); ++fe_idx)
    {
        const fs_path fe_direction_path = fe_base / fe[fe_idx];
        for (size_t chan = 0; chan < MAGNESIUM_NUM_CHANS; ++chan)
        {
            const fs_path fe_path = fe_direction_path / chan;
            UHD_LOG_TRACE(unique_id(), "Adding FE at " << fe_path);
            // Shared TX/RX attributes
            //{
                //auto ad9371_min_gain = (fe_idx == RX_IDX) ? AD9371_MIN_RX_GAIN : AD9371_MIN_TX_GAIN;
                //auto ad9371_max_gain = (fe_idx == RX_IDX) ? AD9371_MAX_RX_GAIN : AD9371_MAX_TX_GAIN;
                //auto ad9371_gain_step = (fe_idx == RX_IDX) ? AD9371_RX_GAIN_STEP : AD9371_TX_GAIN_STEP;
                //auto dsa_min_gain = DSA_MIN_GAIN;
                //auto dsa_max_gain = DSA_MAX_GAIN;
                //auto dsa_gain_step = DSA_GAIN_STEP;
                //auto all_min_gain = (fe_idx == RX_IDX) ? ALL_RX_MIN_GAIN : ALL_TX_MIN_GAIN;
                //auto all_max_gain = (fe_idx == RX_IDX) ? ALL_TX_MAX_GAIN : ALL_TX_MAX_GAIN;
                //auto all_gain_step = 0.5;
                //if (gain_mode == "auto"){
                    //ad9371_min_gain = 0;
                    //ad9371_max_gain = 0;
                    //ad9371_gain_step = 0;
                    //dsa_min_gain = 0;
                    //dsa_max_gain = 0;
                    //dsa_gain_step = 0;
                //}
                //if (gain_mode == "manual")
                //{
                    //all_min_gain = 0 ;
                    //all_max_gain = 0 ;
                    //all_gain_step = 0 ;

                //}
                //auto dir_ = dir[fe_idx];
                ////Create gain property for mykonos
                //auto myk_set_gain_func = [this, chan, dir_](const double gain)
                //{
                    //return this->_myk_set_gain(gain, chan, dir_);
                //};
                //auto myk_get_gain_func = [this, chan, dir_]()
                //{
                    //return this->_myk_get_gain(chan, dir_);
                //};

                //_tree->create<double>(fe_path / "gains" / "ad9371" / "value")
                    //.set(0)
                    //.set_coercer(myk_set_gain_func)
                    //.set_publisher(myk_get_gain_func);
                //_tree->create<meta_range_t>(fe_path / "gains" / "ad9371" / "range")
                    //.set(meta_range_t(ad9371_min_gain, ad9371_max_gain, ad9371_gain_step));
                //// Create gain property for DSA
                //auto dsa_set_gain_func = [this, chan, dir_](const double gain)
                //{
                    //return this->_dsa_set_gain(gain, chan, dir_);
                //};
                //auto dsa_get_gain_func = [this, chan, dir_]()
                //{
                    //return this->_dsa_get_gain(chan, dir_);
                //};
                //_tree->create<double>(fe_path / "gains" / "dsa" / "value")
                    //.set(0)
                    //.set_coercer(dsa_set_gain_func)
                    //.set_publisher(dsa_get_gain_func);
                //_tree->create<meta_range_t>(fe_path / "gains" / "dsa" / "range")
                    //.set(meta_range_t(dsa_min_gain, dsa_max_gain, dsa_gain_step));

                //// Create gain property for all gains
                //auto set_all_gain_func = [this, chan, dir_](const double gain)
                //{
                    //return this->_set_all_gain(gain, chan, dir_);
                //};
                //auto get_all_gain_func = [this, chan, dir_]()
                //{
                    //return this->_get_all_gain(chan, dir_);
                //};
                //_tree->create<double>(fe_path / "gains" / "all" / "value")
                    //.set(0)
                    //.set_coercer(set_all_gain_func)
                    //.set_publisher(get_all_gain_func);
                //_tree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
                    //.set(meta_range_t(all_min_gain, all_max_gain, all_gain_step));

            //}
        }
    }


    // TODO this might be wrong
    if (_master) {
        this->_init_frontend_subtree(_tree->subtree(fe_base), 0, 0);
        std::string slave_slot = (_radio_slot == "A") ? "B" : "D";
        UHD_LOG_TRACE(unique_id(),
            "Also registering props for slave radio " << slave_slot);
        this->_init_frontend_subtree(
                _tree->subtree(fs_path("dboards") / slave_slot), 0, 1);
    }
    // TODO: When we go to one radio per dboard, the above if statement goes
    // away, and instead we have something like this:
    /*
     *for (chan_idx = 0; chan_idx < MAGNESIUM_NUM_CHANS; chan_idx++) {
     *    this->_init_frontend_subtree(
     *        _tree->get_subtree(fe_base), chan_idx, chan_idx);
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
        _tree->create<double>("tick_rate").set(MAGNESIUM_TICK_RATE);
    }
}

