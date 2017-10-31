//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "spi_core_3000.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
    enum slave_select_t {
        SEN_CPLD = 1,
        SEN_TX_LO = 2,
        SEN_RX_LO = 4,
        SEN_PHASE_DAC = 8
    };

    const double MAGNESIUM_TICK_RATE = 125e6; // Hz
    const double MAGNESIUM_RADIO_RATE = 125e6; // Hz
    const double MAGNESIUM_MIN_FREQ = 1e6; // Hz
    const double MAGNESIUM_MAX_FREQ = 6e9; // Hz
    const double AD9371_MIN_RX_GAIN = 0.0; // dB
    const double AD9371_MAX_RX_GAIN = 30.0; // dB
    const double AD9371_RX_GAIN_STEP = 0.5;
    const double DSA_MIN_GAIN = 0; // dB
    const double DSA_MAX_GAIN = 31.5; // dB
    const double DSA_GAIN_STEP = 0.5; // db
    const double AD9371_MIN_TX_GAIN = 0.0; // dB
    const double AD9371_MAX_TX_GAIN = 41.95; // dB
    const double AD9371_TX_GAIN_STEP = 0.05;
    const double ALL_RX_MIN_GAIN = 0.0;
    const double ALL_RX_MAX_GAIN = 61.5;
    const double ALL_RX_GAIN_STEP = 0.5;
    const double ALL_TX_MIN_GAIN = 0.0;
    const double ALL_TX_MAX_GAIN = 73.45;
    const double ALL_TX_GAIN_STEP = 0.5;
    const double MAGNESIUM_CENTER_FREQ = 2.5e9; // Hz
    const char* MAGNESIUM_DEFAULT_RX_ANTENNA = "RX2";
    const char* MAGNESIUM_DEFAULT_TX_ANTENNA = "TX/RX";
    const double MAGNESIUM_DEFAULT_BANDWIDTH = 40e6; // Hz TODO: fix
    const size_t MAGNESIUM_NUM_TX_CHANS = 1;
    const size_t MAGNESIUM_NUM_RX_CHANS = 1;
    const double MAGNESIUM_RX_IF_FREQ = 2.44e9;
    const double MAGNESIUM_TX_IF_FREQ = 1.95e9;
    const double MAGNESIUM_LOWBAND_FREQ = 300e6;
/*
Magnesium Rev C frequency bands:

RX IF frequency is 2.4418 GHz. Have 80 MHz of bandwidth for loband.
TX IF frequency is 1.8-2.1 GHz (1.95 GHz is best).

For RX:
	Band   	  SW2-AB SW3-ABC SW4-ABC SW5-ABCD SW6-ABC SW7-AB SW8-AB MIX
	WB     	  RF1 01 OFF 111 NA  --- NA  ---- RF3 001 RF2 01 RF2 01 0
	LB     	  RF2 10 RF5 100 NA  --- RF3 0010 RF1 100 RF1 10 RF1 10 1
	440-530	  RF2 10 RF2 001 NA  --- RF1 1000 RF1 100 RF2 01 RF2 01 0
	650-1000  RF2 10 RF6 101 NA  --- RF4 0001 RF1 100 RF2 01 RF2 01 0
	1100-1575 RF2 10 RF4 011 NA  --- RF2 0100 RF1 100 RF2 01 RF2 01 0
	1600-2250 RF2 10 RF3 010 RF2 010 NA  ---- RF2 010 RF2 01 RF2 01 0
	2100-2850 RF2 10 RF1 000 RF1 100 NA  ---- RF2 010 RF2 01 RF2 01 0
	2700+     RF3 11 OFF 111 RF3 001 NA  ---- RF2 010 RF2 01 RF2 01 0

For TX:
	Band      SW5-AB SW4-AB SW3-X SW2-ABCD SW1-AB SWTRX-AB MIX
	WB        RF1 10 RF2 01 RF1 0 NA  ---- SHD 00 RF4   11 0
	LB        RF2 01 RF1 10 RF2 1 RF3 0010 RF3 11 RF1   00 1
	<800      RF1 10 RF2 01 RF2 1 RF3 0010 RF3 11 RF1   00 0
	800-1700  RF1 10 RF2 01 RF2 1 RF2 0100 RF2 10 RF1   00 0
	1700-3400 RF1 10 RF2 01 RF2 1 RF1 1000 RF1 01 RF1   00 0
	3400-6400 RF1 10 RF2 01 RF2 1 RF4 0001 SHD 00 RF2   10 0

*/


    const double MAGNESIUM_RX_BAND1_MIN_FREQ = MAGNESIUM_LOWBAND_FREQ;
    const double MAGNESIUM_RX_BAND2_MIN_FREQ = 600e6;
    const double MAGNESIUM_RX_BAND3_MIN_FREQ = 1050e6;
    const double MAGNESIUM_RX_BAND4_MIN_FREQ = 1600e6;
    const double MAGNESIUM_RX_BAND5_MIN_FREQ = 2100e6;
    const double MAGNESIUM_RX_BAND6_MIN_FREQ = 2700e6;

    const double MAGNESIUM_TX_BAND1_MIN_FREQ = MAGNESIUM_LOWBAND_FREQ;
    const double MAGNESIUM_TX_BAND2_MIN_FREQ = 800e6;
    const double MAGNESIUM_TX_BAND3_MIN_FREQ = 1700e6;
    const double MAGNESIUM_TX_BAND4_MIN_FREQ = 3400e6;

    const size_t FPGPIO_MASTER_RADIO = 0;

    /*! Return a valid 'which' string for use with AD9371 API calls
     *
     * These strings take the form of "RX1", "TX2", ...
     */
    std::string _get_which(const direction_t dir,std::string _radio_slot)
    {
        UHD_ASSERT_THROW(dir == RX_DIRECTION or dir == TX_DIRECTION);
        size_t chan = 0;
        if (_radio_slot == "A" or _radio_slot == "C")
        {
             chan = 0;
        }
        if (_radio_slot == "B" or _radio_slot == "D")
        {
            chan = 1;
        }
        UHD_LOG_WARNING("MAGNESIUM_MYKONOS","board slot to chan map " << _radio_slot << " "<<chan)
        return str(boost::format("%s%d")
                   % (dir == RX_DIRECTION ? "RX" : "TX")
                   % (chan+1)
        );
    }
}


/******************************************************************************
 * Structors
 *****************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(magnesium_radio_ctrl)
{
    UHD_LOG_TRACE("MAGNESIUM", "Entering magnesium_radio_ctrl_impl ctor...");
    UHD_LOG_DEBUG("MAGNESIUM", "Note: Running in one-block-per-channel mode!");
    const char radio_slot_name[4] = {'A','B','C','D'};
    _radio_slot = radio_slot_name[get_block_id().get_block_count()];
    UHD_LOG_TRACE("MAGNESIUM", "Radio slot: " << _radio_slot);
    if (_radio_slot == "A" or _radio_slot =="B"){
        _rpc_prefix = "db_0_";
    }
    if (_radio_slot == "C" or _radio_slot=="D")
    {
        _rpc_prefix = "db_1_";
    }
    UHD_LOG_WARNING("MAGNESIUM", "Using RPC prefix `" << _rpc_prefix << "'");

    _init_peripherals();
    _init_defaults();

    fs_path gain_mode_path = _root_path.branch_path()
    / str(boost::format("Radio_%d") % ((get_block_id().get_block_count()/2)*2))
    / "args/0/gain_mode/value";
    UHD_LOG_DEBUG("GAIN_MODE_STRING","Gain mode path " << gain_mode_path);
    std::string gain_mode = _tree->access<std::string>(gain_mode_path).get();
    UHD_LOG_DEBUG("GAIN_MODE_STRING","Gain mode string" << gain_mode);
    //////// REST OF CTOR IS PROP TREE SETUP //////////////////////////////////

    /**** Set up legacy compatible properties ******************************/
    // For use with multi_usrp APIs etc.
    // For legacy prop tree init:
    // TODO: determine DB number
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    const std::vector<uhd::direction_t> dir({ RX_DIRECTION, TX_DIRECTION });
    const std::vector<std::string> fe({ "rx_frontends", "tx_frontends" });
    const std::vector<std::string> ant({ "RX" , "TX" });
    const std::vector<size_t> num_chans({ MAGNESIUM_NUM_RX_CHANS , MAGNESIUM_NUM_TX_CHANS });
    const size_t RX_IDX = 0;
    // const size_t TX_IDX = 1;
    //this->_dsa_set_gain(0.5,0,RX_DIRECTION);
    for (size_t fe_idx = 0; fe_idx < fe.size(); ++fe_idx)
    {
        const fs_path fe_direction_path = fe_base / fe[fe_idx];
        for (size_t chan = 0; chan < num_chans[fe_idx]; ++chan)
        {
            const fs_path fe_path = fe_direction_path / chan;
            UHD_LOG_TRACE("MAGNESIUM", "Adding FE at " << fe_path);
            // Shared TX/RX attributes
            _tree->create<std::string>(fe_path / "name")
                .set(str(boost::format("Magnesium %s %d") % ant[fe_idx] % chan))
                ;
            _tree->create<std::string>(fe_path / "connection")
                .set("IQ")
                ;
            {
                // TODO: fix antenna name
                // Now witness the firepower of this fully armed and operational lambda
                auto dir_ = dir[fe_idx];
                auto coerced_lambda = [this, chan, dir_](const std::string &ant)
                {
                    return this->_myk_set_antenna(ant, chan, dir_);
                };
                auto publisher_lambda = [this, chan, dir_]()
                {
                    return this->_myk_get_antenna(chan, dir_);
                };
                _tree->create<std::string>(fe_path / "antenna" / "value")
                    .set(str(boost::format("%s%d") % ant[fe_idx] % (chan + 1)))
                    .add_coerced_subscriber(coerced_lambda)
                    .set_publisher(publisher_lambda);
                // TODO: fix options
                _tree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
                    .set(std::vector<std::string>(1, str(boost::format("%s%d") % ant[fe_idx] % (chan + 1))));
            }
            {
                auto dir_ = dir[fe_idx];
                auto coerced_lambda = [this, chan, dir_](const double freq)
                {
                    return this->_myk_set_frequency(freq, chan, dir_);
                };
                auto publisher_lambda = [this, chan, dir_]()
                {
                    return this->_myk_get_frequency(chan, dir_);
                };
                _tree->create<double>(fe_path / "freq" / "value")
                    .set(MAGNESIUM_CENTER_FREQ)
                    .set_coercer(coerced_lambda)
                    .set_publisher(publisher_lambda);
                _tree->create<meta_range_t>(fe_path / "freq" / "range")
                    .set(meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ));
            }
            {
                auto ad9371_min_gain = (fe_idx == RX_IDX) ? AD9371_MIN_RX_GAIN : AD9371_MIN_TX_GAIN;
                auto ad9371_max_gain = (fe_idx == RX_IDX) ? AD9371_MAX_RX_GAIN : AD9371_MAX_TX_GAIN;
                auto ad9371_gain_step = (fe_idx == RX_IDX) ? AD9371_RX_GAIN_STEP : AD9371_TX_GAIN_STEP;
                auto dsa_min_gain = DSA_MIN_GAIN;
                auto dsa_max_gain = DSA_MAX_GAIN;
                auto dsa_gain_step = DSA_GAIN_STEP;
                auto all_min_gain = (fe_idx == RX_IDX) ? ALL_RX_MIN_GAIN : ALL_TX_MIN_GAIN;
                auto all_max_gain = (fe_idx == RX_IDX) ? ALL_TX_MAX_GAIN : ALL_TX_MAX_GAIN;
                auto all_gain_step = 0.5;
                if (gain_mode == "auto"){
                    ad9371_min_gain = 0;
                    ad9371_max_gain = 0;
                    ad9371_gain_step = 0;
                    dsa_min_gain = 0;
                    dsa_max_gain = 0;
                    dsa_gain_step = 0;
                }
                if (gain_mode == "manual")
                {
                    all_min_gain = 0 ;
                    all_max_gain = 0 ;
                    all_gain_step = 0 ;

                }
                auto dir_ = dir[fe_idx];
                //Create gain property for mykonos
                auto myk_set_gain_func = [this, chan, dir_](const double gain)
                {
                    return this->_myk_set_gain(gain, chan, dir_);
                };
                auto myk_get_gain_func = [this, chan, dir_]()
                {
                    return this->_myk_get_gain(chan, dir_);
                };

                _tree->create<double>(fe_path / "gains" / "ad9371" / "value")
                    .set(0)
                    .set_coercer(myk_set_gain_func)
                    .set_publisher(myk_get_gain_func);
                _tree->create<meta_range_t>(fe_path / "gains" / "ad9371" / "range")
                    .set(meta_range_t(ad9371_min_gain, ad9371_max_gain, ad9371_gain_step));
                // Create gain property for DSA
                auto dsa_set_gain_func = [this, chan, dir_](const double gain)
                {
                    return this->_dsa_set_gain(gain, chan, dir_);
                };
                auto dsa_get_gain_func = [this, chan, dir_]()
                {
                    return this->_dsa_get_gain(chan, dir_);
                };
                _tree->create<double>(fe_path / "gains" / "dsa" / "value")
                    .set(0)
                    .set_coercer(dsa_set_gain_func)
                    .set_publisher(dsa_get_gain_func);
                _tree->create<meta_range_t>(fe_path / "gains" / "dsa" / "range")
                    .set(meta_range_t(dsa_min_gain, dsa_max_gain, dsa_gain_step));

                // Create gain property for all gains
                auto set_all_gain_func = [this, chan, dir_](const double gain)
                {
                    return this->_set_all_gain(gain, chan, dir_);
                };
                auto get_all_gain_func = [this, chan, dir_]()
                {
                    return this->_get_all_gain(chan, dir_);
                };
                _tree->create<double>(fe_path / "gains" / "all" / "value")
                    .set(0)
                    .set_coercer(set_all_gain_func)
                    .set_publisher(get_all_gain_func);
                _tree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
                    .set(meta_range_t(all_min_gain, all_max_gain, all_gain_step));

            }
            // TODO: set up read/write of bandwidth properties correctly
            if (fe_idx == RX_IDX)
            {
                auto coerced_lambda = [this, chan](const double bw)
                {
                    return this->set_rx_bandwidth(bw, chan);
                };
                auto publisher_lambda = [this, chan]()
                {
                    return this->get_rx_bandwidth(chan);
                };
                _tree->create<double>(fe_path / "bandwidth" / "value")
                    .set(MAGNESIUM_DEFAULT_BANDWIDTH)
                    .set_coercer(coerced_lambda)
                    .set_publisher(publisher_lambda);
            }
            else {
                _tree->create<double>(fe_path / "bandwidth" / "value")
                    .set(MAGNESIUM_DEFAULT_BANDWIDTH);
            }
            _tree->create<meta_range_t>(fe_path / "bandwidth" / "range")
                .set(meta_range_t(MAGNESIUM_DEFAULT_BANDWIDTH, MAGNESIUM_DEFAULT_BANDWIDTH));
        }
    }

    // EEPROM paths subject to change FIXME
    _tree->create<eeprom_map_t>(_root_path / "eeprom").set(eeprom_map_t());

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

magnesium_radio_ctrl_impl::~magnesium_radio_ctrl_impl()
{
    UHD_LOG_TRACE("MAGNESIUM", "magnesium_radio_ctrl_impl::dtor() ");
}

/**************************************************************************
 * Init Helpers
 *************************************************************************/
void magnesium_radio_ctrl_impl::_init_peripherals()
{
    UHD_LOG_TRACE("MAGNESIUM", "Initializing peripherals...");
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
    if (_radio_slot == "A" or _radio_slot == "C") {
        UHD_LOG_TRACE("MAGNESIUM", "Initializing SPI core...");
        _spi = spi_core_3000::make(_get_ctrl(0),
            radio_ctrl_impl::regs::sr_addr(radio_ctrl_impl::regs::SPI),
            radio_ctrl_impl::regs::RB_SPI);
    } else {
        UHD_LOG_TRACE("MAGNESIUM", "Not a master radio, no SPI core.");
    }

    UHD_LOG_TRACE("MAGNESIUM", "Initializing CPLD...");
    UHD_LOG_TRACE("MAGNESIUM", "CPLD path: " << cpld_path);
    if (not _tree->exists(cpld_path)) {
        UHD_LOG_TRACE("MAGNESIUM", "Creating new CPLD object...");
        spi_config_t spi_config;
        spi_config.use_custom_divider = true;
        spi_config.divider = 125;
        spi_config.mosi_edge = spi_config_t::EDGE_RISE;
        spi_config.miso_edge = spi_config_t::EDGE_FALL;
        UHD_LOG_TRACE("MAGNESIUM", "Making CPLD object...");
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
        UHD_LOG_TRACE("MAGNESIUM", "Reusing someone else's CPLD object...");
        _cpld = _tree->access<magnesium_cpld_ctrl::sptr>(cpld_path).get();
    }

    // TODO: Same comment as above applies
    if (_radio_slot == "A" or _radio_slot == "C") {
        UHD_LOG_TRACE("MAGNESIUM", "Initializing TX LO...");
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
        UHD_LOG_TRACE("MAGNESIUM", "Initializing RX LO...");
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
        UHD_LOG_TRACE("MAGNESIUM", "Not a master radio, no LOs.");
    }
    if (not _tree->exists(rx_lo_path)) {
        _tree->create<adf435x_iface::sptr>(rx_lo_path).set(_rx_lo);
    }else
    {
        UHD_LOG_TRACE("MAGNESIUM", "Not a master radio. Getting LO from master" );
        _rx_lo = _tree->access<adf435x_iface::sptr>(rx_lo_path).get();
    }
    if (not _tree->exists(tx_lo_path)) {
        _tree->create<adf435x_iface::sptr>(tx_lo_path).set(_tx_lo);
    }else
    {
        UHD_LOG_TRACE("MAGNESIUM", "Not a master radio. Getting LO from master" );
        _tx_lo = _tree->access<adf435x_iface::sptr>(tx_lo_path).get();
    }

    _gpio.clear(); // Following the as-if rule, this can get optimized out
    for (size_t radio_idx = 0; radio_idx < _get_num_radios(); radio_idx++) {
        UHD_LOG_TRACE("MAGNESIUM",
            "Initializing GPIOs for channel " << radio_idx);
        _gpio.emplace_back(
            gpio_atr::gpio_atr_3000::make(
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
        _fp_gpio = gpio_atr::gpio_atr_3000::make(
                _get_ctrl(0), regs::sr_addr(regs::FP_GPIO), regs::RB_FP_GPIO);
    }
}

void magnesium_radio_ctrl_impl::_init_defaults()
{
    UHD_LOG_TRACE("MAGNESIUM", "Initializing defaults...");
    const size_t num_rx_chans = get_output_ports().size();
    //UHD_ASSERT_THROW(num_rx_chans == MAGNESIUM_NUM_RX_CHANS);
    const size_t num_tx_chans = get_input_ports().size();
    //UHD_ASSERT_THROW(num_tx_chans == MAGNESIUM_NUM_TX_CHANS);

    UHD_LOG_TRACE("MAGNESIUM",
            "Num TX chans: " << num_tx_chans
            << " Num RX chans: " << num_rx_chans);
    UHD_LOG_TRACE("MAGNESIUM",
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


/******************************************************************************
 * API Calls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::set_rate(double rate)
{
    // TODO: implement
    if (rate != get_rate()) {
        UHD_LOG_WARNING("MAGNESIUM",
                "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

void magnesium_radio_ctrl_impl::set_tx_antenna(
        const std::string &ant,
        const size_t chan
) {
    _myk_set_antenna(ant, chan, TX_DIRECTION);
}

void magnesium_radio_ctrl_impl::set_rx_antenna(
        const std::string &ant,
        const size_t chan
) {
    _myk_set_antenna(ant, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_frequency(
        const double freq,
        const size_t chan
) {
    return _myk_set_frequency(freq, chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_frequency(
        const double freq,
        const size_t chan
) {
    return _myk_set_frequency(freq, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    return _myk_set_bandwidth(bandwidth, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_gain(
        const double gain,
        const size_t chan
) {
    return _set_all_gain(gain, chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_gain(
        const double gain,
        const size_t chan
) {

    return _set_all_gain(gain, chan, RX_DIRECTION);
}

std::string magnesium_radio_ctrl_impl::get_tx_antenna(
        const size_t chan
) /* const */ {
    return _myk_get_antenna(chan, TX_DIRECTION);
}

std::string magnesium_radio_ctrl_impl::get_rx_antenna(
        const size_t chan
) /* const */ {
    return _myk_get_antenna(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_tx_frequency(
        const size_t chan
) /* const */ {
    return _myk_get_frequency(chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_frequency(
        const size_t chan
) /* const */ {
    return _myk_get_frequency(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_tx_gain(
    const size_t chan
) /* const */ {
    return _get_all_gain(chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_gain(
    const size_t chan
) /* const */ {
    return _get_all_gain(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_bandwidth(
    const size_t chan
) /* const */ {
    return _myk_get_bandwidth(chan, RX_DIRECTION);
}

size_t magnesium_radio_ctrl_impl::get_chan_from_dboard_fe(
    const std::string &fe, const direction_t dir
) {
    // UHD_LOG_TRACE("MAGNESIUM", "get_chan_from_dboard_fe " << fe << " returns " << boost::lexical_cast<size_t>(fe));
    return boost::lexical_cast<size_t>(fe);
}

std::string magnesium_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan,
    const direction_t dir
) {
    // UHD_LOG_TRACE("MAGNESIUM", "get_dboard_fe_from_chan " << chan << " returns " << std::to_string(chan));
    return std::to_string(chan);
}

double magnesium_radio_ctrl_impl::get_output_samp_rate(size_t port)
{
    return MAGNESIUM_RADIO_RATE;
}

void magnesium_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
) {
    _rpcc = rpcc;
    _block_args = block_args;

    // EEPROM paths subject to change FIXME
    const size_t db_idx = get_block_id().get_block_count();
    _tree->access<eeprom_map_t>(_root_path / "eeprom")
        .add_coerced_subscriber([this, db_idx](const eeprom_map_t& db_eeprom){
            this->_rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
        })
        .set_publisher([this, db_idx](){
            return this->_rpcc->request_with_token<eeprom_map_t>(
                "get_db_eeprom", db_idx
            );
        })
    ;
}

/******************************************************************************
 * Helpers
 *****************************************************************************/
fs_path magnesium_radio_ctrl_impl::_get_fe_path(size_t chan, direction_t dir)
{
    switch (dir)
    {
        case TX_DIRECTION:
            return fs_path("dboards" / _radio_slot / "tx_frontends" / get_dboard_fe_from_chan(chan, TX_DIRECTION));
        case RX_DIRECTION:
            return fs_path("dboards" / _radio_slot / "rx_frontends" / get_dboard_fe_from_chan(chan, RX_DIRECTION));
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void magnesium_radio_ctrl_impl::_update_atr_switches(
    const magnesium_cpld_ctrl::chan_sel_t chan,
    const direction_t dir,
    const std::string &ant
){
    magnesium_cpld_ctrl::rx_sw1_t rx_sw1  = magnesium_cpld_ctrl::RX_SW1_RX2INPUT;
    magnesium_cpld_ctrl::sw_trx_t sw_trx = _sw_trx[chan];

    bool trx_led = false, rx2_led = true;
    //bool tx_pa_enb = true, tx_amp_enb = true, tx_myk_en=true;
    if (ant == "TX/RX" && dir== RX_DIRECTION)
    {
        rx_sw1 = magnesium_cpld_ctrl::RX_SW1_TRXSWITCHOUTPUT;
        sw_trx = magnesium_cpld_ctrl::SW_TRX_RXCHANNELPATH;
        trx_led = true;
        rx2_led = false;
    }
    UHD_LOG_TRACE("MAGNESIUM", "Update all atr related switches for " << dir << " " << ant );
    if (dir == RX_DIRECTION){
        _cpld->set_rx_atr_bits(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::ON,
            rx_sw1,
            trx_led,
            rx2_led,
            true,
            true,
            true,
            true
        );
        _cpld->set_tx_atr_bits(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::IDLE,
            false,
            sw_trx,
            false,
            false,
            false
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            rx_sw1,
            false,
            false,
            false,
            false,
            false,
            true
        );
    }
    if (dir == TX_DIRECTION){
        _cpld->set_tx_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            true,
            sw_trx,
            true,
            true,
            true
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            rx_sw1,
            false,
            false,
            false,
            false,
            false,
            true
        );
    };
}

void magnesium_radio_ctrl_impl::_update_freq_switches(
    const double freq,
    const size_t chan,
    const direction_t dir
){
    UHD_LOG_TRACE("MAGNESIUM", "Update all freq related switches for " << freq);
     // Set filters based on frequency
     if (dir == RX_DIRECTION) {
        if (freq < MAGNESIUM_RX_BAND1_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER0490LPMHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
                magnesium_cpld_ctrl::RX_SW5_FILTER0490LPMHZFROM,
                magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_LOBAND,
                true
            );
        } else if (freq < MAGNESIUM_RX_BAND2_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER0440X0530MHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
                magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        } else if (freq < MAGNESIUM_RX_BAND3_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER0650X1000MHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
                magnesium_cpld_ctrl::RX_SW5_FILTER0650X1000MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        } else if (freq < MAGNESIUM_RX_BAND4_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER1100X1575MHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
                magnesium_cpld_ctrl::RX_SW5_FILTER1100X1575MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        } else if (freq < MAGNESIUM_RX_BAND5_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER1600X2250MHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER1600X2250MHZFROM,
                magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        } else if (freq < MAGNESIUM_RX_BAND6_MIN_FREQ) {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
                magnesium_cpld_ctrl::RX_SW3_FILTER2100X2850MHZ,
                magnesium_cpld_ctrl::RX_SW4_FILTER2100X2850MHZFROM,
                magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        } else {
            _cpld->set_rx_switches(
                magnesium_cpld_ctrl::BOTH,
                magnesium_cpld_ctrl::RX_SW2_UPPERFILTERBANKTOSWITCH4,
                magnesium_cpld_ctrl::RX_SW3_SHUTDOWNSW3,
                magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
                magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
                magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
                magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
                false
            );
        }
    } else {
        //TODO : rememeber to cache  _sw_trx for chan 
        if (freq < MAGNESIUM_TX_BAND1_MIN_FREQ) {
        } else if (freq < MAGNESIUM_TX_BAND2_MIN_FREQ) {
        } else if (freq < MAGNESIUM_TX_BAND3_MIN_FREQ) {
        } else if (freq < MAGNESIUM_TX_BAND4_MIN_FREQ) {
        } else {
        }
    }
    UHD_LOG_INFO("MAGNESIUM", "Update all freq related switches for " << freq <<" finished!.");
}
/******************************************************************************
 * AD9371 Controls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::_myk_set_frequency(
        const double freq,
        const size_t chan,
        const direction_t dir
) {


    // Note: There is only one LO per RX or TX, so changing frequency will
    // affect the adjacent channel in the same direction. We have to make sure
    // that getters will always tell the truth!
    auto which = _get_which(dir,_radio_slot);
    UHD_LOG_TRACE("MAGNESIUM", "requested frequency of " << freq);
    _update_freq_switches(freq,chan,dir);
    double ad9371_freq = freq;
    auto lo_iface = (dir == RX_DIRECTION) ? _rx_lo : _tx_lo;

    if (freq < MAGNESIUM_LOWBAND_FREQ) { // Low band
        UHD_LOG_WARNING("LO BAND", "requested frequency of " << freq);
        double if_freq = (dir == RX_DIRECTION) ? MAGNESIUM_RX_IF_FREQ
                                               : MAGNESIUM_TX_IF_FREQ;
        double lo_freq = if_freq - freq;
        _lo_set_frequency(lo_iface, lo_freq, chan);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, true); // TODO: Find correct value
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, true); // TODO: Find correct value
        lo_iface->commit();
        ad9371_freq = if_freq;
    } else {
        UHD_LOG_WARNING("HI BAND", "requested frequency of " << freq);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, false); // TODO: Find correct value
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, false); // TODO: Find correct value
        lo_iface->commit();
    }

    UHD_LOG_TRACE("MAGNESIUM",
            "Calling " << _rpc_prefix << "set_freq on " << which << " with " << ad9371_freq);
    auto retval = _rpcc->request_with_token<double>(_rpc_prefix + "set_freq", which, ad9371_freq, false);
    UHD_LOG_TRACE("MAGNESIUM",
            _rpc_prefix << "set_freq returned " << retval);


    return retval;
}

double magnesium_radio_ctrl_impl::_myk_set_gain(
        const double gain,
        const size_t chan,
        const direction_t dir
) {
    auto which = _get_which(dir,_radio_slot);
    UHD_LOG_TRACE("MAGNESIUM", "Calling " << _rpc_prefix << "set_gain on " << which << " with " << gain);
    auto retval = _rpcc->request_with_token<double>(_rpc_prefix + "set_gain", which, gain);
    UHD_LOG_TRACE("MAGNESIUM", _rpc_prefix << "set_gain returned " << retval);

    return retval;
    //return 0.0;
}

void magnesium_radio_ctrl_impl::_myk_set_antenna(
        const std::string &ant,
        const size_t chan,
        const direction_t dir
) {
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Attempting to set antenna " << ant << " " << chan << " " << dir );
    magnesium_cpld_ctrl::chan_sel_t chan_sel  = (_radio_slot == "A" or _radio_slot == "C")? magnesium_cpld_ctrl::CHAN1 : magnesium_cpld_ctrl::CHAN2;
    _update_atr_switches(chan_sel,dir,ant);
}

double magnesium_radio_ctrl_impl::_myk_set_bandwidth(const double bandwidth, const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to set bandwidth");
    return get_rx_bandwidth(chan);
}

double magnesium_radio_ctrl_impl::_myk_get_frequency(const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir,_radio_slot);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _rpc_prefix << "get_freq on " << which);
    auto retval = _rpcc->request_with_token<double>(_rpc_prefix + "get_freq", which);
    UHD_LOG_TRACE("MAGNESIUM", _rpc_prefix << "get_freq returned " << retval);
    return retval;
}

double magnesium_radio_ctrl_impl::_myk_get_gain(const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir,_radio_slot);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _rpc_prefix << "get_gain on " << which);
    auto retval = _rpcc->request_with_token<double>(_rpc_prefix + "get_gain", which);
    UHD_LOG_TRACE("MAGNESIUM", _rpc_prefix << "get_gain returned " << retval);
    return retval;
}

std::string magnesium_radio_ctrl_impl::_myk_get_antenna(const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to get antenna");
    return "RX1";
    // CPLD control?
}

double magnesium_radio_ctrl_impl::_myk_get_bandwidth(const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to get bandwidth");
    return MAGNESIUM_DEFAULT_BANDWIDTH;
}

/******************************************************************************
 * ADF4351 Controls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::_lo_set_frequency(
        adf435x_iface::sptr lo_iface,
        const double freq,
        const size_t chan
) {

        UHD_LOG_TRACE("MAGNESIUM", "attempting to tune low band LO to " << freq);
        lo_iface->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
        lo_iface->set_reference_freq(100e6); // FIXME: How to get refclk freq? This can change.
        lo_iface->set_prescaler(adf435x_iface::PRESCALER_4_5);

        double actual_freq = 0.0;
        actual_freq = lo_iface->set_frequency(freq, false); // FIXME: always fractional-n mode
        UHD_LOG_TRACE("MAGNESIUM", "actual low band LO is " << actual_freq);
        lo_iface->set_output_power(adf435x_iface::RF_OUTPUT_A, adf435x_iface::OUTPUT_POWER_2DBM); // TODO: Find correct value
        lo_iface->set_output_power(adf435x_iface::RF_OUTPUT_B, adf435x_iface::OUTPUT_POWER_2DBM); // TODO: Find correct value
        lo_iface->set_charge_pump_current(adf435x_iface::CHARGE_PUMP_CURRENT_0_31MA);

        // TODO: Check for PLL lock
        //sleep(1);
        //auto lock_det = _rpcc->request_with_token<uint16_t>(_slot_prefix + "cpld_peek", 0x12);
        //UHD_LOG_TRACE("MAGNESIUM", "lock detect is " << lock_det);

	return actual_freq;
}
double magnesium_radio_ctrl_impl::_set_all_gain(
    const double gain,
    const size_t chan,
    const direction_t dir
)
{
    UHD_LOG_TRACE("MAGNESIUM", "Setting all gain " << gain);
    // just naively  distributed gain here
    _myk_set_gain(gain/2,chan,dir);
    _dsa_set_gain(gain/2,chan,dir);
    if(dir == RX_DIRECTION or dir == DX_DIRECTION)
    {
        _all_rx_gain = gain;
    }
    if(dir == TX_DIRECTION or dir == DX_DIRECTION)
    {
        _all_tx_gain = gain;
    }

    return gain;
}
double magnesium_radio_ctrl_impl::_get_all_gain(
    const size_t chan,
    const direction_t dir
)
{
    UHD_LOG_TRACE("MAGNESIUM", "Getting all gain ");
    if(dir == RX_DIRECTION )
    {
       return _all_rx_gain;
    }
    if(dir == TX_DIRECTION)
    {
        return _all_tx_gain;
    }

}
double  magnesium_radio_ctrl_impl::_dsa_set_gain(
    const double gain,
    const size_t chan,
    const direction_t dir
) {

    uint32_t dsa_val = 63-2*gain;
    UHD_LOG_TRACE("MAGNESIUM", "Setting dsa gain " << dsa_val);

    _set_dsa_val(chan,  dir, dsa_val);
    if(dir == RX_DIRECTION or dir == DX_DIRECTION)
    {
        _dsa_rx_gain = gain;
    }
    if(dir == TX_DIRECTION or dir == DX_DIRECTION)
    {
        _dsa_tx_gain = gain;
    }
    return gain;
}
double  magnesium_radio_ctrl_impl::_dsa_get_gain(
    const size_t chan,
    const direction_t dir
    ){
        UHD_LOG_TRACE("MAGNESIUM", "Getting dsa gain ");
        if(dir == RX_DIRECTION )
        {
           return _dsa_rx_gain;
        }
        if(dir == TX_DIRECTION)
        {
            return _dsa_tx_gain;
        }

}
/******************************************************************************
 * DSA Controls
 *****************************************************************************/
void magnesium_radio_ctrl_impl::_set_dsa_val(
    const size_t chan,
    const direction_t dir,
    const uint32_t  dsa_val
) {
    UHD_LOG_TRACE("MAGNESIUM", "Setting dsa value" << dsa_val);
    if (dir == RX_DIRECTION or dir == DX_DIRECTION){
        _gpio[chan]->set_gpio_out(dsa_val, 0x3F);
    }
    if (dir == TX_DIRECTION or dir == DX_DIRECTION){
        _gpio[chan]->set_gpio_out(dsa_val, 0x0FC0);
    }
}

UHD_RFNOC_BLOCK_REGISTER(magnesium_radio_ctrl, "MagnesiumRadio");
